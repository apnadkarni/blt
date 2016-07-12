
===============
blt::graph
===============

-------------------------------------------
2D graph for plotting X-Y coordinate data.
-------------------------------------------

:Author: George A. Howlett, Sani R. Nassif
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::graph** *pathName* ?\ *option* *value* ... ?

DESCRIPTION
-----------

The **blt::graph** command creates a graph for plotting two-dimensional
data (X-Y coordinates). It has many configurable components: coordinate
axes, elements, legend, cross hairs, etc.  They allow you to customize the
graph.

INTRODUCTION
------------

The **blt::graph** command creates a new window for plotting
two-dimensional data (X-Y coordinates).  Data points are plotted in a
rectangular plotting area displayed in the center of the new window.
Coordinate axes are drawn in the margins around the plotting area.  By
default, a legend is displayed in the right margin.  A title is displayed
in top margin.

The **blt::graph** widget is composed of several components: coordinate
axes, data elements, legend, cross hairs, pens, postscript, and annotation
markers.

**axis**
  The graph has four standard axes ("x", "x2", "y", and "y2"), but you can
  create and display any number of axes.  Axes control what region of data
  is displayed and how the data is scaled. Each axis consists of the axis
  line, title, major and minor ticks, and tick labels.  Tick labels display
  the value at each major tick.

**crosshairs**
  Cross hairs are used to position the mouse pointer relative to the X and
  Y coordinate axes. Two perpendicular lines, intersecting at the current
  location of the mouse, extend across the plotting area to the coordinate
  axes.

**element**
  An element represents a set of data points. Elements can be plotted with
  a symbol at each data point and lines connecting the points.  The
  appearance of the element, such as its symbol, line width, and color is
  configurable.

**legend**
  The legend displays the name and symbol of each data element. 
  The legend can be drawn in any margin or in the plotting area.

**marker**
  Markers are used annotate or highlight areas of the graph. For example,
  you could use a polygon marker to fill an area under a curve, or a text
  marker to label a particular data point. Markers come in various forms:
  text strings, bitmaps, connected line segments, images, polygons, or
  embedded widgets.

**pen**
  Pens define attributes (both symbol and line style) for elements.  Data
  elements use pens to specify how they should be drawn.  A data element
  may use many pens at once.  Here, the particular pen used for a data
  point is determined from each element's weight vector (see the element's
  **-weight** and **-style** options).

**postscript**
  The widget can generate encapsulated PostScript output. This component
  has several options to configure how the PostScript is generated.

SYNTAX
------

  **blt::graph** *pathName* ?\ *option* *value* ... ?

The **blt::graph** command creates a new window *pathName* and makes it
into a *graph* widget.  At the time this command is invoked, there must not
exist a window named *pathName*, but *pathName*'s parent must exist.
Additional options may be specified on the command line or in the option
database to configure aspects of the graph such as its colors and font.
See the **configure** operation below for the exact details about what
*option* and *value* pairs are valid.

If successful, **blt::graph** returns the path name of the widget.  It also
creates a new TCL command by the same name.  You can use this command to
invoke various operations that query or modify the graph.  The general form
is:

  *pathName* *operation* ?\ *arg* ... ?

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available for the graph are described in 
the `GRAPH OPERATIONS`_ section.

The command can also be used to access components of the graph.

  *pathName* *component* *operation* ?\ *arg*\ ... ?

The operation, now located after the name of the component, is the function
to be performed on that component. Each component has its own set of
operations that manipulate that component.  They will be described below in
their own sections.

GRAPH OPERATIONS
----------------

*pathName* **axis** *operation* ?\ *arg* ... ?
  See the `AXES`_ section.

*pathName* **bar** *elemName* ?\ *option* *value* ... ?
  Creates a new bar element *elemName*.  It's an error if an element
  *elemName* already exists.  See the manual for **blt::barchart** for
  details about what *option* and *value* pairs are valid.

*pathName* **cget** *option*
  Returns the current value of the configuration option given by *option*.
  *Option* may be any option described below for the **configure**
  operation.

*pathName* **configure** ?\ *option* *value* ... ?
  Queries or modifies the configuration options of the graph.  If *option*
  isn't specified, a list describing the current options for *pathName* is
  returned.  If *option* is specified, but not *value*, then a list
  describing *option* is returned.  If one or more *option* and *value*
  pairs are specified, then for each pair, the option *option* is set to
  *value*.  The following options are valid.

  **-aspect** *aspectRatio*
    Force a fixed aspect ratio of width/height, a floating point number.

  **-background** *colorName*
    Specifies the background color of the widget. This includes the margins
    and legend, but not the plotting area.  *ColorName* may be a color name
    or the name of a background object created by the **blt::background**
    command.  The default is "grey85".

  **-borderwidth** *numPixels*
    Specifies the width of the 3-D border around the outside edge of the
    widget.  *NumPixels* may have any of the forms acceptable to
    **Tk_GetPixels**.  The widget's **-relief** option determines if the
    border is to be drawn.  The default is "2".

  **-bottommargin** *numPixels*
    Specifies the height of the bottom margin extending below the
    X-coordinate axis.  *NumPixels* may have any of the forms acceptable to
    **Tk_GetPixels**.  If *numPixels* is "0", the height automatically
    computed.  The default is "0".

  **-bottomvariable** *varName*
    Specifies a TCL variable to be set with the size of the bottom margin.
    Whenever the graph is resized, *varName* will be set with the new value.

  **-bufferelements** *boolean*
    Indicates whether an internal pixmap to buffer the display of data
    elements should be used.  If *boolean* is true, data elements are drawn
    to an internal pixmap.  This option is especially useful when the graph
    is redrawn frequently while the remains data unchanged (for example,
    moving a marker across the plot).  See the `SPEED TIPS`_ section.  The
    default is "1".

  **-cursor** *cursor*
    Specifies the widget's cursor.  The default cursor is "crosshair".

  **-data** *string*
    Sets an arbritrary string.  This isn't used by the widget but may be
    useful for associating data with the graph.  The default is "".

  **-font**  *fontName* 
    Specifies the font of the graph title. The default is "{San Serif} 9".

  **-foreground** *colorName*
    Specifies the color of the graph title. *ColorName* is be a color name.
    The default is "black".

  **-halo** *numPixels* 
    Specifies a maximum distance to consider when searching for the closest
    data point (see the element's **closest** operation below).  Data
    points further than *numPixels* away are ignored.  *NumPixels* may have
    any of the forms acceptable to **Tk_GetPixels**.  The default is
    "0.5i".

  **-height**  *numPixels*
    Specifies the height of widget. *NumPixels* may have any of the forms
    acceptable to **Tk_GetPixels**. The default is "4i".

  **-highlightbackground** *colorName*
    Specifies the color of the traversal highlight region when the
    graph does not have the input focus.  *ColorName* may be a color name
    or the name of a background object created by the **blt::background**
    command.  The default is "grey85".

  **-highlightcolor** *colorName*
    Specifies the color of the traversal highlight region when the
    graph has input focus.   *ColorName* may be a color name
    or the name of a background object created by the **blt::background**
    command. The default is "black".

  **-highlightthickness** *numPixels*
    Specifies a non-negative value for the width of the highlight rectangle
    to drawn around the outside of the widget.  *NumPixels* may have any of
    the forms acceptable to **Tk_GetPixels**.  If *numPixels* is "0.0", no
    focus highlight is drawn around the widget.  The default is "2".

  **-invertxy**  *boolean*
    Indicates whether the location X-axis and Y-axis should be exchanged.
    If *boolean* is true, the X axis runs vertically and Y axis
    horizontally.  The default is "0".

  **-justify**  *justifyName*
    Specifies how the title should be justified when the title contains
    more than one line of text.  *JustifyName* must be "left", "right", or
    "center".  The default is "center".

  **-leftmargin**  *numPixels*
    Specifies the width of the left margin to the left the plot area.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    If *numPixels* is "0", the width automatically computed.  The default
    is "0".

  **-leftvariable** *varName*
    Specifies a TCL variable to be set with the size of the left margin.
    Whenever the graph is resized, *varName* will be set with the new value.

  **-mapelements** *how*
    Specifies what elements to consider when scaling axes. By default the
    extents of elements in the display list are used to compute the scale
    of the axes.  *How* can be one of the following.

    **all**
       Consider all elements.

    **default**
       Consider elements in the display list (see the **element show**
       operation) even if they are hidden. This is the default.

    **visible**
       Consider elements that are not hidden.  When an element is hidden,
       the graph will be automatically rescaled.
       
  **-plotbackground**  *colorName*
    Specifies the background color of the plotting area.  *ColorName* may
    be a color name or the name of a background object created by the
    **blt::background** command. The default is "white".

  **-plotborderwidth**  *numPixels*
    Specifies the width of the 3-D border around the plotting area.  The
    widget's **-plotrelief** option determines if a border is drawn.  The
    default is "2".

  **-plotpadx**  *numPixels*
    Specifies the amount of padding to be added to the left and right sides
    of the plotting area.  *NumPixels* can be a list of one or two screen
    distances.  If *numPixels* has two elements, the left side of the
    plotting area entry is padded by the first distance and the right side
    by the second.  If *numPixels* is just one distance, both the left and
    right sides are padded evenly.  The default is "8".

  **-plotpady**  *numPixels*
    Sets the amount of padding to be added to the top and bottom of the
    plotting area.  *NumPixels* can be a list of one or two screen
    distances.  If *numPixels* has two elements, the top of the plotting
    area is padded by the first distance and the bottom by the second.  If
    *numPixels* is just one distance, both the top and bottom are padded
    evenly.  The default is "8".

  **-plotheight**  *numPixels*
    Specifies the height of the plot area. *NumPixels* may have any of the
    forms acceptable to **Tk_GetPixels**. If *numPixels* is "0", the height
    of the plot area is computed from the availble space in the widget.
    The default is "0".

  **-plotwidth**  *numPixels*
    Specifies the width of the plot area. *NumPixels* may have any of the
    forms acceptable to **Tk_GetPixels**. If *numPixels* is "0", the width
    of the plot area is computed from the availble space in the widget.
    The default is "0".

  **-plotrelief**  *reliefName*
    Specifies the 3-D effect for the plotting area.  *ReliefName* specifies
    how the interior of the plotting area should appear relative to rest of
    the graph; for example, "raised" means the plot should appear to
    protrude from the graph, relative to the surface of the graph.  The
    default is "sunken".

  **-relief**  *reliefName*
    Specifies the 3-D effect for the graph widget.  *ReliefName* specifies
    how the graph should appear relative to widget it is packed int. For
    example, "raised" means the graph should appear to protrude.  The
    default is "flat".

  **-rightmargin**  *numPixels*
    Specifies the width of the right margin to the right the plot area.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    If *numPixels* is "0", the width automatically computed.  The default
    is "0".

  **-rightvariable** *varName*
    Specifies a TCL variable to be set with the size of the right margin.
    Whenever the graph is resized, *varName* will be set with the new value.

  **-stackaxes** *boolean*
    Indicates to stack axes one on top of the other if there are more than
    one axis in a margin. The default is "0".

  **-stretchtofit** *boolean*
    Indicates to stretch the axes to fit the available size of the window.
    This changes the aspect ratio of the graph.
    The default is "1".

  **-takefocus** *focusValue* 
    Specifies how the widget should be handled when movin focus from window
    to window via keyboard traversal (e.g., Tab and Shift-Tab).  If
    *focusValue* is "0", this means that this window should be skipped
    entirely during keyboard traversal. If *focusValue* is "1" this means
    that the this window should always receive the input focus.  An empty
    value "" means that the traversal scripts make the decision whether to
    focus on the window.  The default is "".

  **-title**  *titleString* 
    Specifies the title of the graph. If *titleString* is "" then no title
    will be displayed.  The default is "".

  **-topmargin**  *numPixels* 
    Specifies the height of the top margin extending above the plot area.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    If *numPixels* is "0", the height automatically computed.  The default
    is "0".

  **-topvariable** *varName*
    Specifies a TCL variable to be set with the size of the top margin.
    Whenever the graph is resized, *varName* will be set with the new value.

  **-width**  *numPixels*
    Specifies the width of widget. *NumPixels* may have any of the forms
    acceptable to **Tk_GetPixels**. The default is "5i".

*pathName* **crosshairs** *operation* ?\ *arg* ... ?
  See the `CROSSHAIRS`_ section.

*pathName* **element** *operation* ?\ *arg* ... ?
  See the  `ELEMENTS`_ section.

*pathName* **extents**  *item* 
  Returns the size of a particular item in the graph.  *Item* must be
  either "leftmargin", "rightmargin", "topmargin", "bottommargin",
  "plotwidth", or "plotheight".

*pathName* **inside** *screenX* *sceeenY*
  Returns "1" if *screenX* and *screenY* are is inside the plotting area of
  the graph and "0" otherwise. *ScreenX* and *screenY* are integers
  representing a coordinate on the screen.

*pathName* **invtransform** *screenX* *screenY* ?\ *switches* ... ?
  Transforms screen coordinates into graph coordinates.  *ScreenX* and
  *screenY* are integers representing a coordinate on the screen. By
  default the standard **x** and **y** axes are used.  Returns a list
  containing the x and y graph coordinates.

  **-element**  *elemName* 
    Specifies the name of an element whose axes are used to transform
    *screenX* and *screenY*.

  **-mapx**  *axisName* 
    Specifies the name of the X-axis used to transform *screenY*.

  **-mapy**  *axisName* 
    Specifies the name of the Y-axis used to transform *screenY*.
    
*pathName* **legend** *operation* ?\ *arg* ... ?
  See the `LEGEND`_ section.

*pathName* **line**  *operation* ?\ *arg* ... ?
  The is the same as the **element** operation except that is specifically
  for line elements.  This is the default element type for **blt::graph**
  widgets. See the `ELEMENTS`_ section.

*pathName* **marker** *operation* ?\ *arg* ... ?
  See the `MARKERS`_ section.

*pathName* **pen** *operation* ?\ *arg* ... ?
  See the `PENS`_ section.

*pathName* **region cget** ?\ *option*\ ?
  Returns the current value of the playback configuration option given by
  *option*.  *Option* may be any option described below for the **play
  configure** operation.

*pathName* **region configure**  ?\ *option* *value* ... ?
  Queries or modifies the playback configuration options.  If *option*
  isn't specified, a list describing the current playback options for
  *pathName* is returned.  If *option* is specified, but not *value*, then
  a list describing *option* is returned.  If one or more *option* and
  *value* pairs are specified, then for each pair, the option *option* is
  set to *value*.  The following options are valid.

  **-enable** *boolean*
    Indicates to display only the region of data defined by the **-from**
    and **-to** data point indices.  If *boolean* is true, then the
    region will be displayed.  Otherwise, the entire set of data points
    is plotted.

  **-elements** *list*
    Specifies the elements to display only the region of data points.
    If *list* is "", all elements are affected.
    
  **-from** *fromIndex*
    Specifies the index of the first data point to be played. *FromIndex*
    is a non-negative integer.  Data point indices start from 0.  The
    default is the index of the first data point "0".

  **-to** *toIndex*
    Specifies the index of the last data point to be played. *ToIndex*
    is a non-negative integer.  Data point indices start from 0.  The
    default is the index of the last data point.

*pathName* **region maxpoints** 
  Returns the maximum number of points of the selected elements
  (designated by the **-elements** option).  This is a convenience
  function to determine the limit of the data point indices.

*pathName* **postscript** *operation* ?\ *arg* ... ?
  See the `POSTSCRIPT`_ section.

*pathName* **snap**  *imageName* ?\ *switches* ... ? 
  Draws the graph into *imageName*. *ImageName* is the name of a picture
  or Tk photo image.  This differs from a normal screen snapshot in that 1)
  the graph can be off-screen or obscured by other windows and 2) the
  graph is drawn, not scaled.  For example, the font sizes stay the
  the same. The following switches are available.

  **-format** *imageFormat*
    Specifies how the snapshot is output. *imageFormat* may be one of 
    the following listed below.  The default is "image". 

    **image**
      Saves the output as a BLT **picture** image or Tk **photo** image.
      *ImageName* is the name of a picture or photo image that must already
      have been created.
 
    **wmf**
      Saves an Aldus Placeable Metafile.  *ImageName* represents the
      filename where the metafile is written.  If *imageName* is
      "CLIPBOARD", then output is written directly to the Windows
      clipboard.  This format is available only under Microsoft Windows.
 
    **emf**
      Saves an Enhanced Metafile. *ImageName* represents the filename
      where the metafile is written.  If *imageName* is "CLIPBOARD", then
      output is written directly to the Windows clipboard.  This format is
      available only under Microsoft Windows.

  **-height** *numPixels*
    Specifies the height of the image.  *NumPixels* is a screen distance.
    If *numPixels* is 0, the height of the image will be the current height
    of *pathName*. The default is "0".

  **-width** *numPixels*
    Specifies the width of the image.  *NumPixels* is a screen distance.
    If *numPixels* is 0, the height of the image is the same as the
    graph. The default is "0".

*pathName* **transform** *graphX* *graphY* ?\ *switches* ... ?
  Transforms map coordinates into screen coordinates.  *GraphX* and
  *graphY* are double precision numbers representing a coordinate on the
  graph.  By default the standard **x** and **y** axes are used.  Returns a
  list containing the x and y screen coordinates.

  **-element**  *elemName* 
    Specifies the name of an element whose axes are used to transform
    *graphX* and *graphY*.

  **-mapx**  *axisName* 
    Specifies the name of a X-axis to transform *graphX*. 

  **-mapy**  *axisName* 
    Specifies the name of a Y-axis to transform *graphY*.
    

*pathName* **xaxis**  *operation* ?\ *arg* ... ?
  Same as *pathName* **axis** *operation* **x** ?\ *arg* ... ?.

*pathName* **x2axis**  *operation* ?\ *arg* ... ?
  Same as *pathName* **axis** *operation* **x2** ?\ *arg* ... ?.

*pathName* **yaxis**  *operation* ?\ *arg* ... ?
  Same as *pathName* **axis** *operation* **y** ?\ *arg* ... ?.

*pathName* **y2axis**  *operation* ?\ *arg* ... ?
  Same as *pathName* **axis** *operation* **y2** ?\ *arg* ... ?.

  See the `AXES`_ section.

GRAPH COMPONENTS
----------------

A graph is composed of several components: coordinate axes, data elements,
legend, cross hairs, postscript, and annotation markers. Instead of one big
set of configuration options and operations, the graph is partitioned,
where each component has its own configuration options and operations that
specifically control that aspect or part of the graph.

AXES
~~~~

Four coordinate axes are automatically created: two X-coordinate axes ("x"
and "x2") and two Y-coordinate axes ("y", and "y2").  By default, the axis
"x" is located in the bottom margin, "y" in the left margin, "x2" in the
top margin, and "y2" in the right margin.

An axis consists of the axis line, title, major and minor ticks, and tick
labels.  Major ticks are drawn at uniform intervals along the axis.  Each
tick is labeled with its coordinate value.  Minor ticks are drawn at
uniform intervals within major ticks.

The range of the axis controls what region of data is plotted.  Data points
outside the minimum and maximum limits of the axis are not plotted.  By
default, the minimum and maximum limits are determined from the data, but
you can reset either limit.

You can have several axes. To create an axis, invoke the axis component and
its create operation.

  ::

    # Create a new axis called "tempAxis"
    .g axis create tempAxis

You map data elements to an axis using the element's -mapy and -mapx
configuration options. They specify the coordinate axes an element is
mapped onto.

  ::

    # Now map the tempAxis data to this axis.
    .g element create "e1" -xdata $x -ydata $y -mapy tempAxis

Any number of axes can be displayed simultaneously. They are drawn in the
margins surrounding the plotting area.  The default axes "x" and "y" are
drawn in the bottom and left margins. The axes "x2" and "y2" are drawn in
top and right margins.  By default, only "x" and "y" are shown. Note that
the axes can have different scales.

To display a different axis or more than one axis, you invoke one of
the following components: **xaxis**, **yaxis**, **x2axis**, and
**y2axis**.  Each component has a **use** operation that
designates the axis (or axes) to be drawn in that corresponding
margin: **xaxis** in the bottom, **yaxis** in the left,
**x2axis** in the top, and **y2axis** in the right.

  ::

    # Display the axis tempAxis in the left margin.
    .g yaxis use tempAxis

The **use** operation takes a list of axis names as its last argument.
This is the list of axes to be drawn in this margin.

You can configure axes in many ways. The axis scale can be linear or
logarithmic.  The values along the axis can either monotonically increase
or decrease.  If you need custom tick labels, you can specify a TCL
procedure to format the label any way you wish.  You can control how ticks
are drawn, by changing the major tick interval or the number of minor
ticks.  You can define non-uniform tick intervals, such as for time-series
plots.

Axis configuration options may be also be set by the **option** command.
The resource class is "Axis".  The resource names are the names of the axes
(such as "x" or "x2").

  ::

     option add *Graph.Axis.Color blue option add *Graph.x.LogScale true
     option add *Graph.x2.LogScale false

*pathName* **axis activate** *axisName* 

*pathName* **axis bind** *bindTag* ?\ *eventSequence*\ ?  ?\ *cmdString*\ ?
  Associates *cmdString* with *bindTag* such that whenever the event sequence
  given by *eventSequence* occurs for an axis with this tag, *cmdString* will
  be invoked.  The syntax is similar to the **bind** command except that it
  operates on graph axes, rather than widgets. See the **bind** manual
  page for complete details on *eventSequence* and the substitutions
  performed on *cmdString* before invoking it.

  If all arguments are specified then a new binding is created, replacing
  any existing binding for the same *eventSequence* and *bindTag*.  If the
  first character of *cmdString* is "+" then *command* augments an existing
  binding rather than replacing it.  If no *cmdString* argument is provided
  then the command currently associated with *bindTag* and *eventSequence*
  (it's an error occurs if there's no such binding) is returned.  If both
  *cmdString* and *eventSequence* are missing then a list of all the event
  sequences for which bindings have been defined for *bindTag*.

*pathName* **axis cget** *axisName* *option*
  Returns the current value of the option given by *option* for *axisName*.
  *AxisName* is the name of an axis (such as "x").  *Option* may be any
  option described below for the axis **configure** operation.

*pathName* **axis configure** *axisName* ?\ *option* *value* ... ?
  Queries or modifies the configuration options of *axisName*.  *AxisName*
  is the name of an axis (such as "x").  If *option* isn't specified, a
  list describing all the current options for *axisName* is returned.  If
  *option* is specified, but not *value*, then a list describing *option*
  is returned.  If one or more *option* and *value* pairs are specified,
  then for each pair, the axis option *option* is set to *value*.  The
  following options are valid for axes.

  **-activebackground** *colorName*

  **-activeforeground** *colorName*

  **-activerelief** *relief*

  **-autorange** *windowSize*

  **-background** *colorName*

  **-bindtags** *tagsList*
    Specifies the binding tags for the axis.  *TagsList* is a list of
    binding tags.  The tags and their order will determine how events for
    axes are handled.  Each tag in the list matching the current event
    sequence will have its TCL command executed.  Implicitly the name of
    the axis is always the first tag in the list.  The default value is
    "all".

  **-borderwidth** *numPixels*

  **-checklimits** *boolean*

  **-color** *colorName*
    Sets the color of the axis and tick labels.  The default is "black".

  **-colorbarthickness** *numPixels*

  **-command** *cmdPrefix*
    Specifies a TCL command to be invoked when formatting the axis tick
    labels. *CmdPrefix* is a string containing the name of a TCL proc and
    any extra arguments for the procedure.  This command is invoked for
    each major tick on the axis.  Two additional arguments are passed to
    the procedure: the *pathName* and the current the numeric value of the
    tick.  The procedure returns the formatted tick label.  If "" is
    returned, no label will appear next to the tick.  You can get the
    standard tick labels again by setting *cmdPrefix* to "".  The default
    is "".

    Please note that modifying graph configuration options in the procedure
    may have have unexpected results.

  **-decreasing** *boolean*
    Indicates whether the values along the axis are monotonically
    increasing or decreasing.  If *boolean* is true the axis values will be
    decreasing.  The default is "0".

  **-descending** *boolean*
    Same as the **-decreasing** option above.

  **-divisions** *numMajorTicks*

  **-foreground** *colorName*

  **-grid** *boolean*

  **-gridcolor** *colorName*

  **-griddashes** *dashList*

  **-gridlinewidth** *numPixels*

  **-gridminor** *boolean*

  **-gridminorcolor** *colorName*

  **-gridminordashes** *dashList*

  **-gridminorlinewidth** *numPixels*

  **-hide** *boolean*
    Indicates if the axis is hidden. If *boolean* is true the axis will not
    be displayed on screen.  Element mapped to the *axisName* will
    displayed regardless if the axis is displayed.  The default value is
    "0".

  **-justify** *justifyName*
    Specifies how the axis title should be justified when the axis title
    contains more than one line of text. *JustifyName* must be "left",
    "right", or "center".  The default is "center".

  **-labeloffset** *boolean*

  **-limitscolor** *colorName*

  **-limitsfont** *fontName*

  **-limitsformat** *formatString*
    Specifies a printf-like description to format the minimum and maximum
    limits of the axis.  The limits are displayed at the top/bottom or
    left/right sides of the plotting area.  *FormatString* is a list of one
    or two format descriptions.  If one description is supplied, both the
    minimum and maximum limits are formatted in the same way.  If two, the
    first designates the format for the minimum limit, the second for the
    maximum.  If "" is given as either description, then the that limit
    will not be displayed.  The default is "".

  **-linewidth** *numPixels*
    Specifies the width of the axis and tick lines.  If *numPixels* is "0",
    then no axis is displayed. The default is "1" pixel.

  **-logscale** *boolean*
    Indicates whether the scale of the axis is logarithmic.  If *boolean*
    is true, the axis is logarithmic, otherwise it is linear.  The default
    scale is linear.

    This option is deprecated in favor or the **-scale** option.


  **-loose** *boolean*
    Indicates whether the limits of the axis should fit the data points
    tightly, at the outermost data points, or loosely, at the outer tick
    intervals.  If the axis limit is set with the -min or -max option, the
    axes are displayed tightly.  If *boolean* is true, the axis range is
    "loose".  The default is "0".

  **-majorticks** *tickList*
    Specifies where to display major axis ticks.  You can use this option
    to display ticks at non-uniform intervals.  *TickList* is a list of
    coordinates along the axis designating where major ticks will be drawn.
    No minor ticks are drawn.  If *tickList* is "", major ticks will be
    automatically computed. The default is "".

  **-margin** *marginName*

  **-max** *maxValue*
    Specifies the maximum limit of *axisName*, clipping elements using
    *axisName*.  Any data point greater than *maxValue* is not displayed.
    If *maxValue* is "", the maximum limit is calculated using the largest
    value of all the elements mapped to *axisName*.  The default is "".

  **-min** *minValue*
    Specifies the minimum limit of *axisName*, clipping elements using
    *axisName*. Any data point less than *minValue* is not displayed.  If
    *minValue* is "", the minimum limit is calculated using the smallest
    value of all the elements mapped to *axisName*.  The default is "".

  **-minorticks** *tickList*
    Specifies where to display minor axis ticks.  You can use this option
    to display minor ticks at non-uniform intervals. *TickList* is a list
    of real values, ranging from 0.0 to 1.0, designating the placement of a
    minor tick.  No minor ticks are drawn if the **-majortick** option is
    also set.  If *tickList* is "" then the minor ticks are automatically
    computed. The default is "".

  **-palette** *paletteName*

  **-relief** *relief*

  **-rotate** *numDegrees*
    Specifies the how many degrees to rotate the axis tick labels.
    *NumDegrees* is a real value representing the number of degrees to
    rotate the tick labels.  The default is "0.0".

  **-scale** *scaleValue*
    Specifies the scale of *axisName*. *ScaleValue* can be one of the
    following.

    **linear**
      Indicates that the scale of the axis is linear.  

    **log**
      Indicates that the scale of the axis is logarithmic.  

    **time**
      Indicates that the axis scale is time.  The data values on the axis
      are in assumed to be in seconds.  The tick values will be in
      displayed in a date or time format (years, months, days, hours,
      minutes, or seconds).

  **-scrollcommand** *cmdPrefix*
    Specify the prefix for a command used to communicate with scrollbars
    for this axis.

  **-scrollincrement** *numPixels*
    Sets the maximum limit of the axis scroll region.  If *maxValue* is "",
    the maximum limit is calculated using the largest data value.  The
    default is "".

  **-scrollmax** *maxValue*
    Sets the maximum limit of the axis scroll region.  If *maxValue* is "",
    the maximum limit is calculated using the largest data value.  The
    default is "".

  **-scrollmin** *minValue*
    Sets the minimum limit of axis scroll region.  If *minValue* is "", the
    minimum limit is calculated using the smallest data value.  The default
    is "".

  **-shiftby** *number*

  **-showticks** *boolean*
    Indicates whether axis ticks should be drawn. If *boolean* is true,
    ticks are drawn.  If false, only the axis line is drawn. The default is
    "1".

  **-stepsize** *stepValue*
    Specifies the interval between major axis ticks.  If *stepValue* isn't
    a valid interval (it must be less than the axis range), the request is
    ignored and the step size is automatically calculated.

  **-subdivisions** *numDivisions*
    Indicates how many minor axis ticks are to be drawn.  For example, if
    *numDivisons* is two, only one minor tick is drawn.  If *numDivisions*
    is one, no minor ticks are displayed.  The default is "2".

  **-tickanchor** *anchorName*

  **-tickdirection** *direction*
    Indicates whether the ticks are interior to the plotting area or exterior.
    *Direction* can be any of the following.
    
    **in**
      The ticks extend from the axis line into the plotting area.
    **out**
      The ticks extend away from the plotting area.

    The default is "out".
    
  **-tickfont** *fontName*
    Specifies the font for axis tick labels. The default is "{Sans Serif}
    9".

  **-ticklength** *numPixels*
    Specifies the length of major and minor ticks (minor ticks are half the
    length of major ticks). *NumPixels* may have any of the forms
    acceptable to **Tk_GetPixels**. The default is "8".

  **-timescale** *boolean*
    Indicates whether the scale of the axis scale is time.  If *boolean*
    is true, the axis is time. The default is "0"

    This option is deprecated in favor or the **-scale** option.

  **-title** *titleString*
    Specifies the title of *axisName*. If *titleString* is "", no axis
    title will be displayed.  The default is the *axisName*.

  **-titlealternate** *boolean*
    Indicates to display the axis title in its alternate location.
    Normally the axis title is centered along the axis.  This option places
    the axis either to the right (horizontal axes) or above (vertical axes)
    the axis.  The default is "0".

  **-titlecolor** *colorName*
    Specifies the color of the axis title. The default is "black".

  **-titlefont** *fontName*
    Specifies the font for axis title. The default is "{Sans Serif} 9".

  **-weight** *number*

*pathName* **axis create** *axisName* ?\ *option* *value* ... ?
  Creates a new axis by the name *axisName*.  No axis by the same name can
  already exist. *Option* and *value* are described in above in the **axis
  configure** operation.

*pathName* **axis deactivate** *axisName* 
  Deactivates all the axes matching *pattern*.  Elements whose names
  match any of the patterns given are redrawn using their normal colors.
  *AxisName* is an element name or a tag and may refer to more than one
  element.

*pathName* **axis delete** ?\ *axisName*\ ... ?
  Deletes the one or more axes. Axes are reference counted. *AxisName* is
  not really deleted until it is not longer in use, so it's safe to delete
  axes mapped to elements.

*pathName* **axis focus** ?\ *axisName*\ ?

*pathName* **axis get** *axisName* *value*

*pathName* **axis invtransform** *axisName* *value*
  Performs the inverse transformation, changing the screen coordinate
  *value* to a graph coordinate, mapping the value mapped to *axisName*.
  Returns the graph coordinate.

*pathName* **axis limits** *axisName*
  Returns a list of the minimum and maximum values for *axisName*.  The
  minumum and maximum values are determined from all the elements that are
  mapped to *axisName*.

*pathName* **axis margin** *axisName*

*pathName* **axis names** ?\ *pattern* ... ?
  Returns the names of all the axes in the graph.  If one or more *pattern*
  arguments are provided, then the name of any axis matching *pattern* will
  be returned. *Pattern* is a **glob**\ -style pattern.

*pathName* **axis transform** *axisName* *value*
  Transforms *value* to a screen coordinate by mapping the it to
  *axisName*.  Returns the transformed screen coordinate.

*pathName* **axis type** *axisName*

*pathName* **axis view** *axisName*
  Change the viewable area of this axis. Use as an argument to a
  scrollbar's **-command** option.

  The default axes are "x", "y", "x2", and "y2".  But you can display more
  than four axes simultaneously.  You can also swap in a different axis
  with **use** operation of the special axis components: **xaxis**,
  **x2axis**, **yaxis**, and **y2axis**.

    ::

      .g create axis temp
      .g create axis time
      ...
      .g xaxis use temp
      .g yaxis use time

  Only the axes specified for use are displayed on the screen.

The **xaxis**, **x2axis**, **yaxis**, and **y2axis** components operate on
an axis location rather than a specific axis like the more general **axis**
component does.  They implicitly control the axis that is currently using
to that location.  By default, **xaxis** uses the "x" axis, **yaxis** uses
"y", **x2axis** uses "x2", and **y2axis** uses "y2".  When more than one
axis is displayed in a margin, it represents the first axis displayed.

The following operations are available for axes. They mirror exactly the
operations of the **axis** component.  The *axis* argument must be
**xaxis**, **x2axis**, **yaxis**, or **y2axis**.  This feature is
deprecated since more than one axis can now be used a margin.  You should
only use the **xaxis**, **x2axis**, **yaxis**, and **y2axis** components
with the **use** operation.  For all other operations, use the general
**axis** component instead.

*pathName* *axis* **cget**  *option*

*pathName* *axis* **configure**  ?\ *option* *value* ... ?

*pathName* *axis* **invtransform** *value*

*pathName* *axis* **limits**

*pathName* *axis* **transform** *value*

*pathName* *axis* **use** ?\ *axisName*\ ?  
  Designates the axis *axisName* is to be displayed at this location.
  *AxisName* can not be already in use at another location.  This command
  returns the name of the axis currently using this location.

CROSSHAIRS
~~~~~~~~~~

Cross hairs consist of two intersecting lines (one vertical and one
horizontal) drawn completely across the plotting area.  They are used to
position the mouse in relation to the coordinate axes.  Cross hairs differ
from line markers in that they are implemented using XOR drawing
primitives.  This means that they can be quickly drawn and erased without
redrawing the entire graph.

The following operations are available for cross hairs:

*pathName* **crosshairs cget** *option*
  Returns the current value of the cross hairs configuration option given
  by *option*.  *Option* may be any option described below for the cross
  hairs **configure** operation.

*pathName* **crosshairs configure** ?\ *option* *value* ... ?
  Queries or modifies the configuration options of the cross hairs.  If
  *option* isn't specified, a list describing all the current options for
  the cross hairs is returned.  If *option* is specified, but not *value*,
  then a list describing *option* is returned.  If one or more *option* and
  *value* pairs are specified, then for each pair, the cross hairs option
  *option* is set to *value*.  The following options are available for
  cross hairs.

  **-color**  *colorName* 
    Sets the color of the cross hairs.  The default is "black".

  **-dashes**  *dashList*
    Sets the dash style of the cross hairs lines. *DashList* is a list of
    up to 11 numbers that alternately represent the lengths of the dashes
    and gaps on the cross hair lines.  Each number must be between 1
    and 255.  If *dashList* is "", the cross hairs will be solid lines.

  **-hide**  *boolean*
    Indicates whether cross hairs are drawn. If *boolean* is true, cross
    hairs are not drawn.  The default is "yes".

  **-linewidth**  *numPixels*
    Set the line width of the cross hairs.  The default is "1".

  **-position**  *position* 
    Specifies the screen position where the cross hairs intersect.  *Position*
    must be in the form "@*x*,*y*", where *x* and *y* are the screen
    coordinates of the intersection.

  **-x**  *screenX* 
    Specifies the x-coordinate of screen position where the cross hairs
    intersect.  *ScreenX* is an integer representing a screen
    coordinate (relative to *pathName*).

  **-y**  *screenY* 
    Specifies the y-coordinate of screen position where the cross hairs
    intersect.  *ScreenY* is an integer representing a screen coordinate
    (relative to *pathName*).

  Cross hairs configuration options may be also be set by the **option**
  command.  The resource name and class are "crosshairs" and "Crosshairs"
  respectively.

    ::

      option add *Graph.Crosshairs.LineWidth 2
      option add *Graph.Crosshairs.Color     red


*pathName* **crosshairs off**
  Turns off the cross hairs. 

*pathName* **crosshairs  on**
  Turns on the display of the cross hairs.

*pathName* **crosshairs toggle**
  Toggles the current state of the cross hairs, alternately mapping and
  unmapping the cross hairs.

ELEMENTS
~~~~~~~~

A data element represents a set of data.  It contains x and y vectors
containing the coordinates of the data points.  Elements can be displayed
with a symbol at each data point and lines connecting the points.  Elements
also control the appearance of the data, such as the symbol type, line
width, color etc.

When new data elements are created, they are automatically added to a list
of displayed elements.  The display list controls what elements are drawn
and in what order.

The following operations are available for elements.

*pathName* **element activate** *elemName* ?\ *index* ... ?
  Specifies the data points of element *elemName* to be drawn using active
  foreground and background colors.  *ElemName* is the name of the element
  or a tag and may refer to more than one element. If one or more *index*
  arguments are present, they are the indices of the data points to be
  activated. By default all data points of *elemName* will become active.

*pathName* **element bind** *bindTag* ?\ *eventSequence*\ ?  ?\ *cmdString*\ ? 
  Associates *cmdString* with *bindTag* such that whenever the event sequence
  given by *eventSequence* occurs for an element with this binding tag,
  *cmdString* will be invoked.  The syntax is similar to the **bind** command
  except that it operates on graph elements, rather than widgets. See the
  **bind** manual entry for complete details on *eventSequence* and the
  substitutions performed on *cmdString* before invoking it. *BindTag* is an
  arbitrary string that matches one of the binding tags (see the
  **-bindtags** option) in *elemName*.

  If both *eventSequence* and *cmdString* arguments are present, then a new
  binding is created. If a binding for *eventSequence* and *bindTag*
  already exists it is replaced. But if the first character of *cmdString* is
  "+" then *cmdString* augments an existing binding rather than replacing it.

  If no *cmdString* argument is present then this returns the command
  currently associated with *bindTag* and *eventSequence* (it's an error if
  there's no such binding).  If both *cmdString* and *eventSequence* are
  missing then a list of all the event sequences for which bindings have
  been defined for *bindTag*.

*pathName* **element cget** *elemName* *option*
  Returns the current value of the element configuration option given by
  *option*.  *Option* may be any of the options described below for the
  element **configure** operation.  *ElemName* is an element name or a tag
  but may not reference multiple elements.

*pathName* **element closest** *x* *y* ?\ *option* *value* ... ? ?\ *elemName* ... ?
  Searches for the data point closest to the window coordinates *x* and
  *y*.  By default, all elements are searched.  Hidden elements (see the
  **-hide** option is false) are ignored.  You can limit the search by
  specifying only the elements you want to be considered.  *ElemName* is an
  element name or a tag and may refer to more than one element.  It returns
  a key-value list containing the name of the closest element, the index of
  the closest data point, and the graph-coordinates of the point.  Returns
  "", if no data point within the threshold distance can be found. The
  following *option*-*value* pairs are available.


  **-along**  *direction*
    Search for the closest element using the following criteria:

    **x**
      Find closest element vertically from the given X-coordinate. 

    **y**
      Find the closest element horizontally from the given Y-coordinate. 

    **both**
      Find the closest element for the given point (using both the X and Y
      coordinates).  

  **-halo**  *numPixels*
    Specifies a threshold distance where selected data points are ignored.
    *NumPixels* is a valid screen distance, such as "2" or "1.2i".  If this
    option isn't specified, then it defaults to the value of the graph's
    **-halo** option.

  **-interpolate**  *boolean*
    Indicates whether to consider projections that lie along the line
    segments connecting data points when searching for the closest point.
    If *boolean* is "0", search only for the closest data point.  If
    *boolean* is "1", the search includes projections that lie along the
    line segments connecting the data points.  The default value is "0".

*pathName* **element configure** *elemName* ?\ *option* *value* ... ?
  Queries or modifies the configuration options for *elemName*.  If no
  *option* and *value* are arguments are present, this command returns a
  list describing all the current options for *elemName*.  If *option* is
  specified, but not *value*, then a list describing *option* is returned.
  In both cases, *ElemName* is an element name or a tag but may refer to
  multiple elements.

  If one or more *option* and *value* pairs are specified, then for each
  pair, the element option *option* is set to *value*. *ElemName* is an
  element name or a tag and may refer to more than one element.

  The following options are valid for elements.

  **-activepen**  *penName*
    Specifies pen to use to draw active element.  *PenName* is the name of
    a pen created by the **pen create** operation. If *penName* is "", no
    active elements will be drawn.  The default is "activeLine".

  **-activeforeground**  *colorName* 
    Specifies the foreground color of the element when it is active.  The
    default is "black".

  **-areabackground**  *colorName* 
    FIXME:
    Specifies the background color of the area under the curve. The
    background area color is drawn only for bitmaps (see the
    **-areapattern** option).  If *colorName* is "", the background is
    transparent.  The default is "black".

  **-areaforeground**  *colorName* 
    Specifies the foreground color of the area under the curve.  The default
    is "black".

  **-areapattern**  *pattern* 
    FIXME:
    Specifies how to fill the area under the curve.  *Pattern* may be the
    name of a Tk bitmap, "solid", or "".  If "solid", then the area under the
    curve is drawn with the color designated by the **-areaforeground**
    option.  If a bitmap, then the bitmap is stippled across the area.  Here
    the bitmap colors are controlled by the **-areaforeground** and
    **-areabackground** options.  If *pattern* is "", no filled area is
    drawn.  The default is "".

  **-bindtags**  *tagList*
    Specifies the binding tags for *elemName*.  *TagList* is a list of
    binding tags.  The tags and their order will determine how events are
    handled for *elemName*.  Each tag in the list matching the current
    event sequence will have its TCL command executed.  Implicitly the name
    of the element is always the first tag in the list.  The default value
    is "all".

  **-color**  *colorName* 
    Sets the color of the traces connecting the data points.  

  **-colormap**  *axisName* 

  **-dashes**  *dashList*
    Sets the dash style of element line. *DashList* is a list of up to 11
    numbers that alternately represent the lengths of the dashes and gaps
    on the element line.  Each number must be between 1 and 255.  If
    *dashList* is "", the lines will be solid.

  **-data**  *coordsList*
    Specifies the X-Y coordinates of the data.  *CoordsList* is a list of
    numbers representing the X-Y coordinate pairs of each data point.

  **-errorbars**  *how*
    Specifies how to represent error bars on the graph.

    **x**
      Display error bars  or the x-axis.

    **xhigh**
      Display error bars  or the x-axis.

    **xlow**
      Display error bars  or the x-axis.

    **y**
      Display error bars  or the y-axis.

    **yhigh**
      Display error bars  or the x-axis.

    **ylow**
      Display error bars  or the x-axis.

    **both**
      Display error bars  or the y-axis.

  **-errorbarcolor**  *colorName*
    Specifies the color of the error bars.  If *colorName* is "defcolor",
    then the color will be the same as the **-color** option.  The default
    is "defcolor".

  **-errorbarlinewidth**  *numPixels*
    Sets the width of the lines for error bars.  If *numPixels* is "0", no
    connecting error bars will be drawn.  The default is "1".

  **-errorbarcapwidth**  *numPixels*
    Specifies the width of the cap for error bars.  If *numPixels* is "0",
    then the cap width is computed from the symbol size. The default is
    "0".

  **-fill**  *colorName* 
    Sets the interior color of symbols.  If *colorName* is "", then the
    interior of the symbol is transparent.  If *colorName* is "defcolor",
    then the color will be the same as the **-color** option.  The default is
    "defcolor".

  **-hide**  *boolean*
    Indicates whether the element is displayed.  The default is "no".

  **-label**  *labelString*
    Sets the element's label in the legend.  If *labelString* is "", the
    element will have no entry in the legend.  The default label is the
    element's name.

  **-legendrelief**  *relief* 

  **-linewidth**  *numPixels* 
    Sets the width of the connecting lines between data points.  If
    *numPixels* is "0", no connecting lines will be drawn between symbols.
    The default is "0".

  **-mapx**  *axisName*
    Selects the X-axis to map the element's X-coordinates onto.  *AxisName*
    must be the name of an axis.  The default is "x".

  **-mapy**  *axisName*
    Selects the Y-axis to map the element's Y-coordinates onto.  *AxisName*
    must be the name of an axis. The default is "y".

  **-maxsymbols**  *numSymbols*
    Specifies the maximum number of symbols to use when displaying *elemName*.
    If *numSymbols* is "0", then there will be a symbol for every data point
    in *elemName*.

  **-offdash**  *colorName*
    Sets the color of the stripes when traces are dashed (see the
    **-dashes** option).  If *colorName* is "", then the "off" pixels will
    represent gaps instead of stripes.  If *colorName* is "defcolor", then
    the color will be the same as the **-color** option.  The default is
    "defcolor".

  **-outline**  *colorName* 
    Sets the color of the outline for symbols.  If *colorName* is "", then
    no outline is drawn. If *colorName* is "defcolor", then the color will
    be the same as the **-color** option.  The default is "defcolor".

  **-outlinewidth**  *numPixels* 
    Sets the width of the outline for symbols.  If *numPixels* is "0", no
    outline will be drawn. The default is "1".

  **-pen**  *penName*
    Specifies the pen to use for *elemName*.

  **-pixels**  *numPixels*
    Sets the size of the symbols for *elemName*.  If *NumPixels* is "0", no
    symbols will be drawn.  The default is "0.125i".

  **-reduce**  *tolerance*

  **-scalesymbols**  *boolean* 
    If *boolean* is true, the size of the symbols drawn for *elemName* will
    change with scale of the X-axis and Y-axis.  At the time this option is
    set, the current ranges of the axes are saved as the normalized scales
    (i.e scale factor is 1.0) and the element is drawn at its designated size
    (see the **-pixels** option).  As the scale of the axes change, the
    symbol will be scaled according to the smaller of the X-axis and Y-axis
    scales.  If *boolean* is false, the element's symbols are drawn at the
    designated size, regardless of axis scales.  The default is "0".

  **-smooth**  *smoothValue* 
    Specifies how connecting line segments are drawn between data points.
    *SmoothValue* can be one of the following.

    **catrom**
      This is the same as **natural**.

    **cubic**
      Multiple segments are generated between data points using a cubic
      spline.  The abisscas (X-coordinates) must be monotonically
      increasing.

    **linear**
      A single line segment is drawn, connecting both data points. 

    ***natural**
      This is the same as **cubic**.

    **none**
      This is the same as **linear**.

    **parametriccubic**
      This is similar to **cubic** but abscissas (X-coordinates) do not
      need to be monotonically increasing.  The location on the splice is
      roughly computed by arc length.  

    **parametricquadratic**
      This is similar to **quadratic** but abscissas (X-coordinates) do not
      need to be monotonically increasing.  The location on the splice is
      roughly computed by arc length.  

    **quadratic**
      Multiple segments are generated between data points using a quadratic
      spline. The abisscas (X-coordinates) must be monotonically
      increasing.

    **step**
      Two line segments are drawn. The first is a horizontal line segment
      that steps the next X-coordinate.  The second is a vertical line,
      moving to the next Y-coordinate.

  **-state**  *state* 
    Specifies the state of the marker. *State* can be "normal" or "disabled".

  **-styles**  *stylesList* 
    Specifies what pen to use based on the range of weights given.
    *StylesList* is a list of style specifications. Each style
    specification, in turn, is a list consisting of a pen name, and
    optionally a minimum and maximum range.  Data points whose weight (see
    the **-weight** option) falls in this range, are drawn with this pen.
    If no range is specified it defaults to the index of the pen in the
    list.  Note that this affects only symbol attributes. Line attributes,
    such as line width, dashes, etc. are ignored.

  **-symbol**  *symbolName* 
    Specifies the symbol for data points.  *SymbolName* can be one of
    the following.

    **arrow**
      Draw an arrow symbol.  This is basically an inverted triangle. The
      symbol has fill and outline colors.

    **circle**
      Draw a circle symbol.  The symbol has fill and outline colors.

    **diamond**
      Draw a diamond symbol.  The symbol has fill and outline colors.

    **plus**
      Draw a plus symbol.  The symbol has fill and outline colors.

    **cross**
      Draw a cross symbol.  The symbol has fill and outline colors.

    **scross**
      Draw an cross symbol as two lines.  The symbol only has an outline
      color.

    **splus**
      Draw an plus symbol as two lines.  The symbol only has an outline
      color.

    **square**
      Draw a square symbol.  The symbol has fill and outline colors.

    **triangle**
      Draw a triangle symbol.  The symbol has fill and outline colors.

    *imageName*
      Draw an image *imageName*.  The image may contain transparent
      pixels.  

  **-trace**  *direction* 
    Indicates whether connecting lines between data points (whose
    X-coordinate values are either increasing or decreasing) are drawn.
    *Direction* must be "increasing", "decreasing", or "both".  For example,
    if *direction* is "increasing", connecting lines will be drawn only
    between those data points where X-coordinate values are monotonically
    increasing.  If *direction* is "both", connecting lines will be draw
    between all data points.  The default is "both".

  **-valueanchor**  *anchor* 

  **-valuecolor**  *colorName* 

  **-valuefont**  *fontName* 

  **-valueformat**  *formatString* 

  **-valuerotate**  *numDegrees* 

  **-weights**  *data* 
    Specifies the weights of the individual data points.  This, with the
    list pen styles (see the **-styles** option), controls how data points
    are drawn.  *WVec* is the name of a BLT vector or a list of numeric
    expressions representing the weights for each data point.

  **-x**  *data* 
    Same as the **-xdata** option.

  **-xdata**  *data* 
    Specifies the X-coordinates of the data.  *XVec* is the name of a BLT
    vector or a list of numeric expressions.

  **-xerror**  *data* 
    Specifies the X-coordinates of the data.  *XVec* is the name of a BLT
    vector or a list of numeric expressions.

  **-xhigh**  *data* 
    Specifies the X-coordinates of the data.  *XVec* is the name of a BLT
    vector or a list of numeric expressions.

  **-xlow**  *data* 
    Specifies the X-coordinates of the data.  *XVec* is the name of a BLT
    vector or a list of numeric expressions.

  **-y**  *data* 
    Same as the **-ydata** option.

  **-ydata**  *data* 
    Specifies the Y-coordinates of the data.  *Data* is the name of a BLT
    vector or a list of numeric expressions.

  **-yerror**  *data* 
    Specifies the X-coordinates of the data.  *XVec* is the name of a BLT
    vector or a list of numeric expressions.

  **-yhigh**  *data* 
    Specifies the X-coordinates of the data.  *XVec* is the name of a BLT
    vector or a list of numeric expressions.

  **-ylow**  *data* 
    Specifies the X-coordinates of the data.  *XVec* is the name of a BLT
    vector or a list of numeric expressions.

*pathName* **element create** *elemName* ?\ *option* *value* ... ?
  Creates a new element *elemName*.  It's an error if an element named
  *elemName* already exists.  If additional arguments are present, they
  specify options valid for the element. See the **configure** operation
  for a description of *option* and *value*.

  Element configuration options may also be set by the **option** command.
  The resource class is "Element". The resource name is the name of the
  element.

    ::

       option add *Graph.Element.symbol line
       option add *Graph.e1.symbol line


*pathName* **element deactivate** ?\ *elemName* ... ?
  Deactivates all the elements matching *pattern*.  Elements whose names
  match any of the patterns given are redrawn using their normal colors.
  *ElemName* is an element name or a tag and may refer to more than one
  element.
  
*pathName* **element delete** ?\ *elemName* ... ?
  Deletes one or more elements in *pathName*.  *ElemName* is an element
  name or a tag and may refer to more than one element.

*pathName* **element exists** *elemName*
  Returns "1" if the element *elemName* exists and "0" otherwise.
  *ElemName* is an element name or a tag but may not reference multiple
  elements.
  
*pathName* **element find** *elemName* *x1* *y1* *x2* *y2*
  Finds the data points of *elemName* that are contained in the rectangular
  region defined by the given screen coordinates. *X1*, *y1* and *x2*, *y2*
  represent opposite corners of the rectangle.  The indices of the data
  points within the region are returned.

*pathName* **element find** *elemName* *centerX* *centerY* *numPixels* 
  Finds the data points of *elemName* that are contained within a circular
  region. *CenterX* and *centerY* are the screen coordinates for the center
  of the circle.  *NumPixels* is the radius of the circle.  It is a valid
  screen distance, such as "2" or "1.2i".  The indices of the data points
  within the circle are returned.

*pathName* **element get** *elemName* 

*pathName* **element lower** ?\ *elemName* ... ?
  Lowers *elemName* in the stacking order so that it will be drawn below
  other elements in *pathName*.  *ElemName* is an element name or a tag and
  may refer to more than one element.

*pathName* **element names** ?\ *pattern* ... ?
  Returns the names of all the elements in the graph.  If one or more
  *pattern* arguments are provided, then the name of any element matching
  *pattern* will be returned. *Pattern* is a **glob**\ -style pattern.

*pathName* **element nearest** *screenX* *screenY* ?\ *option* *value* ... ? ?\ *elemName* ... ?
  Searches for the data point closest to the given screen
  coordinate. *ScreenX* and *screenY* are screen coordinates (relative to
  *pathName*).  Hidden elements (see the **-hide** option) and points
  outside of a threshold distance (see the **-halo** option) are ignored.
  If no *elemName* arguments are given then all elements are searched.  If
  there are one or more *elemName* arguments they are the elements to be
  searched.  *ElemName* is the name of an element or a tag that may refer
  to multiple elements.  If a closes element is found, this command returns
  a list of key-value pairs containing 1) the name of the closest
  element, 2) the index of the closest data point, and 3) the
  graph-coordinates of the point.  If no element is found, the empty string
  "" is returned.  The following options are valid.

  **-along**  *direction*
    Searches for the closest element using the following criteria:

    **x**
      Find the closest element along the x-axis (vertically from *screenX*). 

    **y**
      Find the closest element along the y-axis (horizontally from *screenY*). 

    **both**
      Find the closest element from the point *screenX*,\ *screenY*.  This is
      the default.

  **-halo**  *numPixels*
    Specifies a threshold distance beyond which data points are ignored.
    *NumPixels* is a valid screen distance, such as "2" or "1.2i".  If this
    option isn't specified, then it defaults to the value of the graph
    widget's **-halo** option.

  **-interpolate**  *boolean*
    Indicates to consider points that lie on the line segments between
    points.  If *boolean* is true, the search includes projections that lie
    along the line segments connecting data points. If *boolean* is false,
    search only for the closest data point.  The default value is "0".

*pathName* **element raise** ?\ *elemName* ... ?
  Raises *elemName* in the stacking order so that it is drawn on top of
  other elements in *pathName*.  *ElemName* is an element name or a tag and
  may refer to more than one element.

*pathName* **element show** ?\ *elemNameList*\ ?  
  Sets or gets the stacking order of elements in *pathName*.  If no
  *elemNameList* argument is given, this command returns the names of
  elements in order in which they are drawn (lowest to highest).  This list
  also determines what elements are used to compute the axes scale.

  Elements that are later in the list will be drawn over elements the occur
  earlier.  Each item in *elemNameList* is a the name of an element or a
  tag (the tag may refer to more than one element). It there are duplicate
  elements in the list, only the first occurance is relevant.

*pathName* **element tag add** *tag* ?\ *elemName* ...\ ? 
  Adds the tag to one or more elements in *pathName* *ElemName* is an
  element name or a tag. *Tag* is an arbitrary string but can't be a
  built-in tag (like "all"). It is not an error if *elemName* already has
  the tag. If no *elemName* arguments are present, *tag* is added to
  *pathName* but refers to no elements.  This is useful for creating empty
  element tags.

*pathName* **element tag delete**  *tag* ?\ *elemName* ...\ ? 
  Removes the tag from one or more elements in *pathName*.  *ElemName* is
  an element name or a tag.  *Tag* is an arbitrary string but can't be a
  built-in tag (like "all"). The built-in tag "all" and can't be
  deleted.

*pathName* **element tag exists**  *tag* ?\ *elemName* ...\ ? 
  Indicates if any element in *pathName* has the tag.  *Tag* is an
  arbitrary string.  Returns "1" if the tag exists, "0" otherwise.  By
  default all elements are searched. But if one or more *elemName*
  arguments are present, then if the tag is found in any *elemName*, "1" is
  returned. *ElemName* may be an element name or a tag and may refer to
  multiple columns (example: "all").

*pathName* **element tag forget**  ?\ *tag* ...\ ? 
  Remove one or more tags from all the elements in *pathName*. *Tag* is an
  arbitrary string but can't be one of the built-in tags ("all").

*pathName* **element tag get** *elemName* ?\ *pattern* ...\ ? 
  Returns the tags for *elemName*. *ElemName* may be an element name of or
  a tag, but may not represent more than one element. By default all tags
  for *elemName* are returned.  But if one or more *pattern* arguments are
  present, then any tag that matching one of the patterns will be returned.
  *Pattern* is a **glob**\ -style pattern.

*pathName* **element tag names** ?\ *pattern* ...\ ? 
  Returns the names of element tags in *pathName*. By default all element
  tags are returned. But if one or more *pattern* arguments are present,
  then any tag matching one of the patterns will be returned. *Pattern* is
  a **glob**\ -style pattern.

*pathName* **element tag search** ?\ *tag* ...\ ? 
  Returns the names of elements that have one or more the named tags. *Tag*
  is an arbitrary string.

*pathName* **element tag set** *elemName* ?\ *tag* ...\?
  Adds one or more tags to *elemName*. *ElemName* is the name of an element
  in *pathName* or a tag that may refer to multiple elements (example:
  "all"). *Tag* is an arbitrary string but can't be one of the built-in
  tags ("all").

*pathName* **element tag unset** *elemName* ?\ *tag*... ?
  Remove one or more tags from *elemName*. *ElemName* is the name of an
  element or a tag and may refer to more than one element. *Tag* is an
  arbitrary string.  You can't use the built-in tag "all".

*pathName* **element type** *elemName*
  Returns the type of *elemName*.  The possible element types are "bar",
  "line", "strip", and "contour". *ElemName* is an element name or a tag
  but may not reference multiple elements.

LEGEND
~~~~~~

The legend displays a list of the data elements.  Each entry consists of
the element's symbol and label.  The legend can appear in any margin (the
default location is in the right margin).  It can also be positioned
anywhere within the plotting area.

Legend configuration options may also be set by the **option** command.
The resource name and class are "legend" and "Legend" respectively.

  ::

      option add *Graph.legend.Foreground blue
      option add *Graph.Legend.Relief     raised


The following operations are valid for the legend.

*pathName* **legend activate** ?\ *elemName*\ ?

  Selects legend entries to be drawn using the active legend colors and
  relief.  All entries whose element names match *pattern* are selected.
  To be selected, the element name must match only one *pattern*.

*pathName* **legend bbox** ?\ *elemName*\ ?

  Returns the bounding box of *elemName* in the legend. The bounding box is
  the region of the entry's label in the legend. *ElemName* can be the name
  of the entry, or it's an index in the legend.  The returned bounding box is
  a list of 4 numbers: x and y coordinates of the upper left corner and
  width and height of the entry.

*pathName* **legend bind** *bindTag* ?\ *eventSequence*\ ?  ?\ *cmdString*\ ? 
  Associates *cmdString* with *bindTag* such that whenever the event sequence
  given by *eventSequence* occurs for a legend entry with this tag,
  *cmdString* will be invoked.  Implicitly the element names in the entry are
  tags.  The syntax is similar to the **bind** command except that it
  operates on legend entries, rather than widgets. See the **bind** manual
  entry for complete details on *eventSequence* and the substitutions
  performed on *cmdString* before invoking it.

  If all arguments are specified then a new binding is created, replacing
  any existing binding for the same *eventSequence* and *bindTag*.  If the
  first character of *cmdString* is "+" then *command* augments an existing
  binding rather than replacing it.  If no *cmdString* argument is provided
  then the command currently associated with *bindTag* and *eventSequence*
  (it's an error occurs if there's no such binding) is returned.  If both
  *cmdString* and *eventSequence* are missing then a list of all the event
  sequences for which bindings have been defined for *bindTag*.

*pathName* **legend cget** *option*

  Returns the current value of a legend configuration option.  *Option* may
  be any option described below in the legend **configure** operation.

*pathName* **legend configure** ?\ *option* *value* ... ? 

  Queries or modifies the configuration options for the legend.  If
  *option* isn't specified, a list describing the current legend options
  for *pathName* is returned.  If *option* is specified, but not *value*,
  then a list describing *option* is returned.  If one or more *option* and
  *value* pairs are specified, then for each pair, the legend option
  *option* is set to *value*.  The following options are valid for the
  legend.

  **-activebackground**  *colorName*
    Sets the background color for active legend entries.  All legend entries
    marked active (see the legend **activate** operation) are drawn using
    this background color.

  **-activeborderwidth**  *numPixels*
    Sets the width of the 3-D border around the outside edge of the active
    legend entries.  The default is "2".

  **-activeforeground**  *colorName*
    Sets the foreground color for active legend entries.  All legend entries
    marked as active (see the legend **activate** operation) are drawn using
    this foreground color.

  **-activerelief**  *relief* 
    Specifies the 3-D effect desired for active legend entries.  *Relief*
    denotes how the interior of the entry should appear relative to the
    legend; for example, "raised" means the entry should appear to protrude
    from the legend, relative to the surface of the legend.  The default is
    "flat".

  **-anchor**  *anchor*
    Tells how to position the legend relative to the positioning point for
    the legend.  This is dependent on the value of the **-position** option.
    The default is "center".

    **left**  **right**
      The anchor describes how to position the legend vertically.  

    **top**  **bottom**
      The anchor describes how to position the legend horizontally.  

    **plotarea**
      The anchor specifies how to position the legend relative to the
      plotting area. For example, if *anchor* is "center" then the legend is
      centered in the plotting area; if *anchor* is "ne" then the legend will
      be drawn such that occupies the upper right corner of the plotting
      area.

    **@**\ *x*\ **,**\ *y*
      The anchor specifies how to position the legend relative to the
      positioning point. For example, if *anchor* is "center" then the legend
      is centered on the point; if *anchor* is "n" then the legend will be
      drawn such that the top center point of the rectangular region occupied
      by the legend will be at the positioning point.

  **-background**  *colorName*
    Sets the background color of the legend. If *colorName* is "", the legend
    background with be transparent.

  **-bindtags**  *tagList*
    Specifies the binding tags for legend entries.  *TagList* is a list of
    binding tag names.  The tags and their order will determine how events
    are handled for legend entries.  Each tag in the list matching the
    current event sequence will have its TCL command executed. The default
    value is "all".

  **-borderwidth**  *numPixels*
    Sets the width of the 3-D border around the outside edge of the legend
    (if such border is being drawn; the **relief** option determines this).
    The default is "2" pixels.

  **-columns**  *numColumns* 

  **-command**  *cmdString* 

  **-exportselection**  *boolean* 

  **-focusdashes**  *dashList* 

  **-focusforeground**  *colorName* 

  **-font**  *fontName* 
    *FontName* specifies a font to use when drawing the labels of each
    element into the legend.  The default is "{San Serif} 9".

  **-foreground** *colorName*
    Sets the foreground color of the text drawn for the element's label.  The
    default is "black".

  **-hide**  *boolean*
    Indicates whether the legend should be displayed. If *boolean* is true,
    the legend will not be drawn.  The default is "no".

  **-ipadx**  *numPixels* 
    Sets the amount of internal padding to be added to the width of each
    legend entry.  *NumPixels* can be a list of one or two screen distances.
    If *numPixels* has two elements, the left side of the legend entry is
    padded by the first distance and the right side by the second.  If
    *numPixels* is just one distance, both the left and right sides are
    padded evenly.  The default is "2".

  **-ipady**  *numPixels*
    Sets an amount of internal padding to be added to the height of each
    legend entry.  *NumPixels* can be a list of one or two screen distances.  If
    *numPixels* has two elements, the top of the entry is padded by the first
    distance and the bottom by the second.  If *numPixels* is just one distance,
    both the top and bottom of the entry are padded evenly.  The default is
    "2".

  **-nofocusselectbackground**  *colorName*

  **-nofocusselectforeground**  *colorName*

  **-padx**  *numPixels*
    Sets the padding to the left and right exteriors of the legend.
    *NumPixels* can be a list of one or two screen distances.  If *numPixels*
    has two elements, the left side of the legend is padded by the first
    distance and the right side by the second.  If *numPixels* has just one
    distance, both the left and right sides are padded evenly.  The default
    is "4".

  **-pady**  *numPixels*
    Sets the padding above and below the legend.  *NumPixels* can be a list
    of one or two screen distances.  If *NumPixels* has two elements, the
    area above the legend is padded by the first distance and the area below
    by the second.  If *numPixels* is just one distance, both the top and
    bottom areas are padded evenly.  The default is "0".

  **-position**  *pos*
    Specifies where the legend is drawn. The **-anchor** option also affects
    where the legend is positioned.  If *pos* is "left", "left", "top", or
    "bottom", the legend is drawn in the specified margin.  If *pos* is
    "plotarea", then the legend is drawn inside the plotting area at a
    particular anchor.  If *pos* is in the form "@*x*,*y*", where *x* and
    *y* are the window coordinates, the legend is drawn in the plotting area
    at the specified coordinates.  The default is "right".

  **-raised**  *boolean*
    Indicates whether the legend is above or below the data elements.  This
    matters only if the legend is in the plotting area.  If *boolean* is
    true, the legend will be drawn on top of any elements that may overlap
    it. The default is "no".

  **-relief**  *relief*
    Specifies the 3-D effect for the border around the legend.  *Relief*
    specifies how the interior of the legend should appear relative to the
    graph; for example, "raised" means the legend should appear to protrude
    from the graph, relative to the surface of the graph.  The default is
    "sunken".

  **-rows**  *numRows*

  **-selectbackground**  *colorName*

  **-selectcommand**  *cmdString*

  **-selectforeground**  *colorName*

  **-selectmode**  *modeName*

  **-selectrelief**  *relief*

  **-takefocus**  *how*

  **-title**  *titleString*

  **-titlecolor**  *colorName*

  **-titlefont**  *fontName*

*pathName* **legend curselection** 

*pathName* **legend deactivate** *pattern*...

  Selects legend entries to be drawn using the normal legend colors and
  relief.  All entries whose element names match *pattern* are selected.
  To be selected, the element name must match only one *pattern*.

*pathName* **legend focus** 

*pathName* **legend get** *pos*

  Returns the name of the element whose entry is at the screen position
  *pos* in the legend.  *Pos* must be in the form "@*x*,*y*", where *x* and
  *y* are window coordinates.  If the given coordinates do not lie over a
  legend entry, "" is returned.

*pathName* **legend icon** *elemName* *imageName*

*pathName* **legend selection archor** *elemName*

*pathName* **legend selection clear** *firstElem* ?\ *lastElem*\ ?

*pathName* **legend selection clearall** 

*pathName* **legend selection includes** *elemName*

*pathName* **legend selection mark** *elemName*

*pathName* **legend selection present** *elemName*

*pathName* **legend selection set**  *firstElem* ?\ *lastElem*\ ?

*pathName* **legend selection toggle**  *firstElem* ?\ *lastElem*\ ?

PENS
~~~~

Pens define attributes (both symbol and line style) for elements.  Pens
mirror the configuration options of data elements that pertain to how
symbols and lines are drawn.  Data elements use pens to determine how they
are drawn.  A data element may use several pens at once.  In this case, the
pen used for a particular data point is determined from each element's
weight vector (see the element's **-weight** and **-style** options).

One pen, called "activeLine", is automatically created.  It's used as the
default active pen for elements. So you can change the active attributes
for all elements by simply reconfiguring this pen.

  ::

    .g pen configure "activeLine" -color green


You can create and use several pens. To create a pen, invoke the **pen
create** operation.

  ::
  
    .g pen create myPen


You map pens to a data element using either the element's 
**-pen** or **-activepen** options.

  ::

    .g element create "line1" -xdata $x -ydata $tempData -pen myPen


An element can use several pens at once. This is done by specifying the
name of the pen in the element's style list (see the **-styles** option).

  ::

    .g element configure "line1" -styles { myPen 2.0 3.0 }

This says that any data point with a weight between 2.0 and 3.0 is to be
drawn using the pen "myPen".  All other points are drawn with the element's
default attributes.

The following operations are available for pens.

*pathName* **pen cget** *penName* *option*

  Returns the current value of the option given by *option* for *penName*.
  *Option* may be any option described below for the pen **configure**
  operation.

*pathName* **pen configure**  *penName* ?\ *option* *value* ... ?

  Queries or modifies the configuration options of *penName*.  If *option*
  isn't specified, a list describing the current options for *penName* is
  returned.  If *option* is specified, but not *value*, then a list
  describing *option* is returned.  If one or more *option* and *value*
  pairs are specified, then for each pair, the pen option *option* is set
  to *value*.  The following options are valid for pens.

  **-color**  *colorName* 
    Sets the color of the traces connecting the data points.  

  **-dashes**  *dashList*
    Sets the dash style of element line. *DashList* is a list of up to 11
    numbers that alternately represent the lengths of the dashes and gaps
    on the element line.  Each number must be between 1 and 255.  If
    *dashList* is "", the lines will be solid.

  **-errorbars**  *how*
    Specifies how to represent error bars on the graph.

    **x**
      Display error bars  or the x-axis.

    **xhigh**
      Display error bars  or the x-axis.

    **xlow**
      Display error bars  or the x-axis.

    **y**
      Display error bars  or the y-axis.

    **yhigh**
      Display error bars  or the x-axis.

    **ylow**
      Display error bars  or the x-axis.

    **both**
      Display error bars  or the y-axis.

  **-errorbarcolor**  *colorName*
    Specifies the color of the error bars.  If *colorName* is "defcolor",
    then the color will be the same as the **-color** option.  The default
    is "defcolor".

  **-errorbarlinewidth**  *numPixels*
    Sets the width of the lines for error bars.  If *numPixels* is "0", no
    connecting error bars will be drawn.  The default is "1".

  **-errorbarcapwidth**  *numPixels*
    Specifies the width of the cap for error bars.  If *numPixels* is "0",
    then the cap width is computed from the symbol size. The default is
    "0".

  **-fill**  *colorName* 
    Sets the interior color of symbols.  If *colorName* is "", then the
    interior of the symbol is transparent.  If *colorName* is "defcolor",
    then the color will be the same as the **-color** option.  The default
    is "defcolor".

  **-linewidth**  *numPixels* 
    Sets the width of the connecting lines between data points.  If
    *numPixels* is "0", no connecting lines will be drawn between symbols.
    The default is "0".

  **-offdash**  *colorName*
    Sets the color of the stripes when traces are dashed (see the
    **-dashes** option).  If *colorName* is "", then the "off" pixels will
    represent gaps instead of stripes.  If *colorName* is "defcolor", then
    the color will be the same as the **-color** option.  The default is
    "defcolor".

  **-outline**  *colorName* 
    Sets the color or the outline around each symbol.  If *colorName* is
    "", then no outline is drawn. If *colorName* is "defcolor", then the
    color will be the same as the **-color** option.  The default is
    "defcolor".

  **-outlinewidth**  *numPixels* 
    Sets the width of the outline bordering each symbol.  If *numPixels* is
    "0", no outline will be drawn. The default is "1".

  **-pixels**  *numPixels*
    Sets the size of symbols.  If *numPixels* is "0", no symbols will be
    drawn.  The default is "0.125i".

  **-showvalues**  *boolean* 
  
  **-symbol**  *symbol* 
    Specifies the symbol for data points.  *SymbolName* can be one of
    the following.

    **arrow**
      Draw an arrow symbol.  This is basically an inverted triangle. The
      symbol has fill and outline colors.

    **circle**
      Draw a circle symbol.  The symbol has fill and outline colors.

    **diamond**
      Draw a diamond symbol.  The symbol has fill and outline colors.

    **plus**
      Draw a plus symbol.  The symbol has fill and outline colors.

    **cross**
      Draw a cross symbol.  The symbol has fill and outline colors.

    **scross**
      Draw an cross symbol as two lines.  The symbol only has an outline
      color.

    **splus**
      Draw an plus symbol as two lines.  The symbol only has an outline
      color.

    **square**
      Draw a square symbol.  The symbol has fill and outline colors.

    **triangle**
      Draw a triangle symbol.  The symbol has fill and outline colors.

    *imageName*
      Draw an image *imageName*.  The image may contain transparent
      pixels.  

  **-type**  *elemType* 
    Specifies the type of element the pen is to be used with.  This option
    should only be employed when creating the pen.  This is for those that
    wish to mix different types of elements (bars and lines) on the same
    graph.  The default type is "line".

  **-valueanchor**  *anchor* 

  **-valuecolor**  *colorName* 

  **-valuefont**  *fontName* 

  **-valueformat**  *formatString* 

  **-valuerotate**  *numDegrees* 

*pathName* **pen create**  *penName*  ?\ *option* *value* ... ?
  Creates a new pen by the name *penName*.  No pen by the same name can
  already exist. *Option* and *value* are described in above in the pen
  **configure** operation.

  Pen configuration options may be also be set by the **option** command.
  The resource class is "Pen".  The resource names are the names of the pens.

    ::

       option add *Graph.Pen.Color  blue
       option add *Graph.activeLine.color  green


*pathName* **pen delete** ?\ *penName* ... ?
  Deletes one or more pens. *PenName* is the name of a pen created by the
  **pen create** operation.  A pen is not really deleted until it is not
  longer in use, so it's safe to delete pens mapped to elements.

*pathName* **pen names** ?\ *pattern* ... ?
  Returns the names of all the pens in the graph.  If one or more
  *pattern* arguments are provided, then the name of any pen matching
  *pattern* will be returned. *Pattern* is a **glob**\ -style pattern.

*pathName* **pen type** *penName*

POSTSCRIPT
~~~~~~~~~~

The graph can generate encapsulated PostScript output.  There are several
configuration options you can specify to control how the plot will be
generated.  You can change the page dimensions and borders.  The plot
itself can be scaled, centered, or rotated to landscape.  The PostScript
output can be written directly to a file or returned through the
interpreter.

The following postscript operations are available.

*pathName* **postscript cget** *option* 
  Returns the current value of the postscript option given by *option*.
  *Option* may be any option described below for the postscript
  **configure** operation.

*pathName* **postscript  configure** ?\ *option* *value* ... ?
  Queries or modifies the configuration options for PostScript generation.
  If *option* isn't specified, a list describing the current postscript
  options for *pathName* is returned.  If *option* is specified, but not
  *value*, then a list describing *option* is returned.  If one or more
  *option* and *value* pairs are specified, then for each pair, the
  postscript option *option* is set to *value*.  The following postscript
  options are available.

  **-center**  *boolean*
    Indicates whether the plot should be centered on the PostScript page.  If
    *boolean* is false, the plot will be placed in the upper left corner of
    the page.  The default is "1".

  **-colormap**  *varName*
    *VarName* must be the name of a global array variable that specifies a
    color mapping from the X color name to PostScript.  Each element of
    *varName* must consist of PostScript code to set a particular color value
    (e.g. "1.0 1.0 0.0 setrgbcolor"").  When generating color information in
    PostScript, the array variable *varName* is checked if an element of the
    name as the color exists. If so, it uses its value as the PostScript
    command to set the color.  If this option hasn't been specified, or if
    there isn't an entry in *varName* for a given color, then it uses the
    red, green, and blue intensities from the X color.

  **-colormode**  *mode*
    Specifies how to output color information.  *Mode* must be either "color"
    (for full color output), "gray" (convert all colors to their gray-scale
    equivalents) or "mono" (convert foreground colors to black and background
    colors to white).  The default mode is "color".

  **-comments**  *list*
    Specifies comments to be added to the PostScript output.

  **-decorations**  *boolean*
    Indicates whether PostScript commands to generate color backgrounds and
    3-D borders will be output.  If *boolean* is false, the background will
    be white and no 3-D borders will be generated. The default is "1".

  **-fontmap**  *varName*
    *VarName* must be the name of a global array variable that specifies a
    font mapping from the X font name to PostScript.  Each element of
    *varName* must consist of a TCL list with one or two elements; the name
    and point size of a PostScript font.  When outputting PostScript commands
    for a particular font, the array variable *varName* is checked to see if
    an element by the specified font exists.  If there is such an element,
    then the font information contained in that element is used in the
    PostScript output.  (If the point size is omitted from the list, the
    point size of the X font is used).  Otherwise the X font is examined in
    an attempt to guess what PostScript font to use.  This works only for
    fonts whose foundry property is *Adobe* (such as Times, Helvetica,
    Courier, etc.).  If all of this fails then the font defaults to
    "Helvetica-Bold".

  **-footer**  *formatString*
    Specifies a footer to displayed at the bottom of the page.
    
  **-greyscale**  *boolean*
    Writes the image out in greyscale.
    
  **-height**  *numPica*
    Sets the height of the plot.  This lets you print the graph with a height
    different from the one drawn on the screen.  If *numPica* is 0, the
    height is the same as the widget's height.  The default is "0".

  **-landscape**  *boolean*
    If *boolean* is true, this specifies the printed area is to be rotated 90
    degrees.  In non-rotated output the X-axis of the printed area runs along
    the short dimension of the page ("portrait" orientation); in rotated
    output the X-axis runs along the long dimension of the page
    ("landscape" orientation).  Defaults to "0".

  **-level**  *psLevel*

  **-padx**  *numPica*
    Sets the horizontal padding for the left and right page borders.  The
    borders are exterior to the plot.  *NumPica* can be a list of one or two
    page distances.  If *numPica* has two elements, the left border is padded
    by the first distance and the right border by the second.  If *numPica* has
    just one distance, both the left and right borders are padded evenly.
    The default is "1i".

  **-pady**  *numPica* 
    Sets the vertical padding for the top and bottom page borders. The
    borders are exterior to the plot.  *NumPica* can be a list of one or two
    page distances.  If *numPica* has two elements, the top border is padded
    by the first distance and the bottom border by the second.  If *numPica*
    has just one distance, both the top and bottom borders are padded evenly.
    The default is "1i".

  **-paperheight**  *numPica*
    Sets the height of the postscript page.  This can be used to select
    between different page sizes (letter, A4, etc).  The default height is
    "11.0i".

  **-paperwidth**  *numPica*
    Sets the width of the postscript page.  This can be used to select
    between different page sizes (letter, A4, etc).  The default width is
    "8.5i".

  **-width**  *numPica*
    Sets the width of the plot.  This lets you generate a plot of a width
    different from that of the widget.  If *numPica* is 0, the width is the
    same as the widget's width.  The default is "0".

  Postscript configuration options may be also be set by the **option**
  command.  The resource name and class are "postscript" and "Postscript"
  respectively.

    ::

      option add *Graph.postscript.Decorations false
      option add *Graph.Postscript.Landscape   true


*pathName* **postscript output** ?\ *fileName*\ ? ?\ *option* *value* ... ?
  Outputs a file of encapsulated PostScript.  If a *fileName* argument
  isn't present, the command returns the PostScript. If any
  *option*-*value* pairs are present, they set configuration options
  controlling how the PostScript is generated.  *Option* and *value* can be
  anything accepted by the postscript **configure** operation above.

MARKERS
~~~~~~~

Markers are simple drawing procedures used to annotate or highlight areas
of the graph.  Markers have various types: text strings, bitmaps, images,
connected lines, windows, or polygons.  They can be associated with a
particular element, so that when the element is hidden or un-hidden, so is
the marker.  By default, markers are the last items drawn, so that data
elements will appear in behind them.  You can change this by configuring
the **-under** option.

**Bitmap Markers**

  A bitmap marker displays a bitmap.  The size of the bitmap is controlled
  by the number of coordinates specified.  If two coordinates, they specify
  the position of the top-left corner of the bitmap.  The bitmap retains
  its normal width and height.  If four coordinates, the first and second
  pairs of coordinates represent the corners of the bitmap.  The bitmap
  will be stretched or reduced as necessary to fit into the bounding
  rectangle.

**Image Markers**

  A image marker displays an image.  

**Line Markers**

  A line marker displays one or more connected line segments.  

**Polygon Markers**

  A polygon marker displays a closed region described as two or more
  connected line segments.  It is assumed the first and last points are
  connected.

**Text Markers**

  A text marker displays a string of characters on one or more lines of
  text.  Embedded newlines cause line breaks.  They may be used to annotate
  regions of the graph.

**Window Markers**

  A window marker displays a widget at a given position.  

Markers, in contrast to elements, don't affect the scaling of the
coordinate axes.  They can also have *elastic* coordinates (specified by
"-Inf" and "Inf" respectively) that translate into the minimum or maximum
limit of the axis.  For example, you can place a marker so it always
remains in the lower left corner of the plotting area, by using the
coordinates "-Inf","-Inf".

The following operations are available for markers.

*pathName* **marker** bind *bindTag* ?\ *eventSequence*\ ?  ?\ *cmdString*\ ? 
  Associates *cmdString* with *bindTag* such that whenever the event sequence
  given by *eventSequence* occurs for a marker with this tag, *cmdString*
  will be invoked.  The syntax is similar to the **bind** command except
  that it operates on graph markers, rather than widgets. See the **bind**
  manual entry for complete details on *eventSequence* and the
  substitutions performed on *cmdString* before invoking it.

  If all arguments are specified then a new binding is created, replacing
  any existing binding for the same *eventSequence* and *bindTag*.  If the
  first character of *cmdString* is "+" then *cmdString* augments an existing
  binding rather than replacing it.  If no *cmdString* argument is provided
  then the command currently associated with *bindTag* and *eventSequence*
  (it's an error occurs if there's no such binding) is returned.  If both
  *cmdString* and *eventSequence* are missing then a list of all the event
  sequences for which bindings have been defined for *bindTag*.

*pathName* **marker cget** *markerName* *option*

  Returns the current value of the marker configuration option given by
  *option*.  *Option* may be any option described below in the
  **configure** operation.

*pathName* **marker configure** *markerName* ?\ *option* *value* ... ?

  Queries or modifies the configuration options for markers.  If *option*
  isn't specified, a list describing the current options for *markerId* is
  returned.  If *option* is specified, but not *value*, then a list
  describing *option* is returned.  If one or more *option* and *value*
  pairs are specified, then for each pair, the marker option *option* is
  set to *value*.

  Each type of marker also has its own specific options.  They are
  described in the **marker create** operation for each type below.

*pathName* **marker create bitmap** ?\ *option* *value* ... ?

  Creates a bitmap marker. This command returns the marker name in the form
  "marker0","marker1", etc.  You can use the **-name** option to specify
  you own name for the marker.

  Bitmap marker configuration options may also be set by the **option**
  command.  The resource class is "BitmapMarker".  The resource name is the
  name of the marker.

    ::

      option add *Graph.BitmapMarker.Foreground white
      option add *Graph.m1.Background     blue

  There may be many *option*-*value* pairs.  Each sets a configuration
  options for the marker.  These same *option*-*value* pairs may be used
  with the **marker configure** operation.

  The following options are specific to bitmap markers:

  **-anchor**  *anchorName*

  **-background**  *colorName*
    Same as the **-fill** option.

  **-bindtags**  *tagList*
    Specifies the binding tags for the marker.  *TagList* is a list of
    binding tag names.  The tags and their order will determine how events
    for markers are handled.  Each tag in the list matching the current
    event sequence will have its TCL command executed.  Implicitly the name
    of the marker is always the first tag in the list.  The default value
    is "all".

  **-bitmap**  *bitmapName*
    Specifies the bitmap to be displayed.  If *bitmapName* is "", the
    marker will not be displayed.  The default is "".

  **-coords**  *coordsList*
    Specifies the coordinates of the marker.  *CoordsList* is a list of 2
    or 4 numbers.  If *coordsList* has four numbers, they represent the
    corners of the bitmap. The bitmap will be stretched to fit the region.
    If *coordsList* has two number, they represent the upper left corner of
    bitmap.  The bitmap will have its noraml size.

  **-element**  *elemName*
    Links the visibility of *markerName with the *elemName*.  The marker is
    drawn only if the element is also currently displayed (see the
    element's **show** operation).  If *elemName* is "", the marker is
    always drawn.  The default is "".

  **-fill**  *colorName*
    Sets the background color of the bitmap.  If *colorName* is the empty
    string, no background will be transparent.  The default background
    color is "".

  **-foreground**  *colorName* 
    Same as the **-outline** option.

  **-hide**  *boolean* 
    Indicates whether the marker is drawn. If *boolean* is true, the marker
    is not drawn.  The default is "no".

  **-mapx**  *axisName* 
    Specifies the X-axis to map the marker's X-coordinates onto.
    *AxisName* must the name of an axis.  The default is "x".

  **-mapy**  *axisName*
    Specifies the Y-axis to map the marker's Y-coordinates onto.
    *AxisName* must the name of an axis.  The default is "y".

  **-mask**  *maskBitmapName*
    Specifies a mask for the bitmap to be displayed. This mask is a bitmap
    itself, denoting the pixels that are transparent.  If *maskBitmapName*
    is "", all pixels of the bitmap will be drawn.  The default is "".

  **-name**  *string*
    Changes the name for the marker.  The name *string* can not already be
    used by another marker.  If this option isn't specified, the marker's
    name is uniquely generated.

  **-outline**  *colorName*
    Sets the foreground color of the bitmap. The default value is "black".

  **-rotate**  *numDegrees*
    Sets the rotation of the bitmap.  *NumDegrees* is a real number
    representing the angle of rotation in degrees.  The marker is first
    rotated and then placed according to its anchor position.  The default
    rotation is "0.0".

  **-state**  *state*
    Specifies the state of the marker. *State* can be "normal" or "disabled".

  **-under**  *boolean*
    Indicates whether the marker is drawn below/above data elements.  If
    *boolean* is true, the marker is be drawn underneath the data element
    symbols and lines.  Otherwise, the marker is drawn on top of the
    element.  The default is "0".

  **-xoffset**  *numPixels*
    Specifies a screen distance to offset the marker horizontally.
    *NumPixels* is a valid screen distance, such as "2" or "1.2i".  The
    default is "0".

  **-yoffset**  *numPixels*
    Specifies a screen distance to offset the markers vertically.
    *NumPixels* is a valid screen distance, such as "2" or "1.2i".  The
    default is "0".

*pathName* **marker create image** ?\ *option* *value* ... ?

  Creates an image marker. This command returns the marker name in the form
  "marker0","marker1", etc.  You can use the **-name** option to specify
  you own name for the marker.  

  Image marker configuration options may also be set by the **option**
  command.  The resource class is "ImageMarker".  The resource name is the
  name of the marker.

    ::

      option add *Graph.ImageMarker.image image1

  There may be many *option*-*value* pairs, each sets a configuration
  option for the marker.  These same *option*-*value* pairs may be used
  with the **marker configure** operation.

  The following options are specific to image markers:

  **-anchor**  *anchor*
    *Anchor* tells how to position the image relative to the positioning
    point for the image. For example, if *anchor* is "center" then the
    image is centered on the point; if *anchor* is "n" then the image will
    be drawn such that the top center point of the rectangular region
    occupied by the image will be at the positioning point.  This option
    defaults to "center".

  **-bindtags**  *tagList*
    Specifies the binding tags for the marker.  *TagList* is a list of
    binding tag names.  The tags and their order will determine how events
    for markers are handled.  Each tag in the list matching the current
    event sequence will have its TCL command executed.  Implicitly the name
    of the marker is always the first tag in the list.  The default value
    is "all".

  **-coords**  *coordsList*
    Specifies the coordinates of the marker.  *CoordsList* is a list of
    graph coordinates.  The number of coordinates required is dependent on
    the type of marker.  Text, image, and window markers need only two
    coordinates (an X-Y coordinate).  Bitmap markers can take either two or
    four coordinates (if four, they represent the corners of the
    bitmap). Line markers need at least four coordinates, polygons at least
    six.  If *coordsList* is "", the marker will not be displayed.  The
    default is "".

  **-element**  *elemName*
    Links the visibility of *markerName with the *elemName*.  The marker is
    drawn only if the element is also currently displayed (see the
    element's **show** operation).  If *elemName* is "", the marker is
    always drawn.  The default is "".

  **-filter**  *filterName*

  **-hide**  *boolean* 
    Indicates whether the marker is drawn. If *boolean* is true, the marker
    is not drawn.  The default is "no".

  **-image**  *imageName*
    Specifies the image to be drawn.  If *imageName* is "", the marker will
    not be drawn.  The default is "".

  **-mapx**  *axisName* 
    Specifies the X-axis to map the marker's X-coordinates onto.
    *AxisName* must the name of an axis.  The default is "x".

  **-mapy**  *axisName*
    Specifies the Y-axis to map the marker's Y-coordinates onto.
    *AxisName* must the name of an axis.  The default is "y".

  **-name**  *string*
    Changes the name for the marker.  The name *string* can not already be
    used by another marker.  If this option isn't specified, the marker's
    name is uniquely generated.

  **-state**  *state*
    Specifies the state of the marker. *State* can be "normal" or "disabled".

  **-under**  *boolean*
    Indicates whether the marker is drawn below/above data elements.  If
    *boolean* is true, the marker is be drawn underneath the data element
    symbols and lines.  Otherwise, the marker is drawn on top of the
    element.  The default is "0".

  **-xoffset**  *numPixels*
    Specifies a screen distance to offset the marker horizontally.
    *NumPixels* is a valid screen distance, such as "2" or "1.2i".  The
    default is "0".

  **-yoffset**  *numPixels*
    Specifies a screen distance to offset the markers vertically.
    *NumPixels* is a valid screen distance, such as "2" or "1.2i".  The
    default is "0".

*pathName* **marker create line** ?\ *option* *value* ... ?

  Creates a line marker. This command returns the marker name in the form
  "marker0","marker1", etc.  You can use the **-name** option to specify
  you own name for the marker.

  Line marker configuration options may also be set by the **option**
  command.  The resource class is "LineMarker".  The resource name is the
  name of the marker.

    ::

      option add *Graph.LineMarker.fill blue


  There may be many *option*-*value* pairs, each sets a configuration
  option for the marker.  These same *option*-*value* pairs may be used
  with the **marker configure** operation.

  The following options are specific to line markers:

  **-bindtags**  *tagList*
    Specifies the binding tags for the marker.  *TagList* is a list of
    binding tag names.  The tags and their order will determine how events
    for markers are handled.  Each tag in the list matching the current
    event sequence will have its TCL command executed.  Implicitly the name
    of the marker is always the first tag in the list.  The default value
    is "all".

  **-cap**  *capStyle*

  **-coords**  *coordsList*
    Specifies the coordinates of the marker.  *CoordsList* is a list of
    graph coordinates.  The number of coordinates required is dependent on
    the type of marker.  Text, image, and window markers need only two
    coordinates (an X-Y coordinate).  Bitmap markers can take either two or
    four coordinates (if four, they represent the corners of the
    bitmap). Line markers need at least four coordinates, polygons at least
    six.  If *coordsList* is "", the marker will not be displayed.  The
    default is "".

  **-dashes**  *dashList*
    Sets the dash style of the line. *DashList* is a list of up to 11
    numbers that alternately represent the lengths of the dashes and gaps
    on the line.  Each number must be between 1 and 255.  If *dashList* is
    "", the marker line will be solid.

  **-dashoffset**  *numPixels*

  **-element**  *elemName*
    Links the visibility of *markerName with the *elemName*.  The marker is
    drawn only if the element is also currently displayed (see the
    element's **show** operation).  If *elemName* is "", the marker is
    always drawn.  The default is "".

  **-fill**  *colorName*
    Sets the background color of the line.  This color is used with striped
    lines (see the **-dashes** option). If *colorName* is the empty string,
    no background color is drawn (the line will be dashed, not striped).
    The default background color is "".

  **-hide**  *boolean* 
    Indicates whether the marker is drawn. If *boolean* is true, the marker
    is not drawn.  The default is "no".

  **-join**  *joinStyle*

  **-linewidth**  *numPixels*
    Sets the width of the lines.  The default width is "0".

  **-mapx**  *axisName* 
    Specifies the X-axis to map the marker's X-coordinates onto.
    *AxisName* must the name of an axis.  The default is "x".

  **-mapy**  *axisName*
    Specifies the Y-axis to map the marker's Y-coordinates onto.
    *AxisName* must the name of an axis.  The default is "y".

  **-name**  *string*
    Changes the name for the marker.  The name *string* can not already be
    used by another marker.  If this option isn't specified, the marker's
    name is uniquely generated.

  **-outline**  *colorName*
    Sets the foreground color of the line. The default value is "black".

  **-stipple**  *bitmapName*
    Specifies a stipple pattern used to draw the line, rather than a solid
    line.  *BitmapName* specifies a bitmap to use as the stipple pattern.
    If *bitmapName* is "", then the line is drawn in a solid fashion. The
    default is "".

  **-under**  *boolean*
    Indicates whether the marker is drawn below/above data elements.  If
    *boolean* is true, the marker is be drawn underneath the data element
    symbols and lines.  Otherwise, the marker is drawn on top of the
    element.  The default is "0".

  **-xoffset**  *numPixels*
    Specifies a screen distance to offset the marker horizontally.
    *NumPixels* is a valid screen distance, such as "2" or "1.2i".  The
    default is "0".

  **-yoffset**  *numPixels*
    Specifies a screen distance to offset the markers vertically.
    *NumPixels* is a valid screen distance, such as "2" or "1.2i".  The
    default is "0".

*pathName* **marker create polygon** ?\ *option* *value* ... ?

  Creates a polygon marker. This command returns the marker name in the
  form "marker0","marker1", etc.  You can use the **-name** option to
  specify you own name for the marker.

  Polygon marker configuration options may also be set by the **option**
  command.  The resource class is "PolygonMarker".  The resource name is
  the name of the marker.

    ::

      option add *Graph.PolygonMarker.fill blue

  There may be many *option*-*value* pairs, each sets a configuration
  option for the marker.  These same *option*-*value* pairs may be used
  with the **marker configure** command to change the marker's
  configuration.  The following options are supported for polygon markers:

  **-bindtags**  *tagList*
    Specifies the binding tags for the marker.  *TagList* is a list of
    binding tag names.  The tags and their order will determine how events
    for markers are handled.  Each tag in the list matching the current
    event sequence will have its TCL command executed.  Implicitly the name
    of the marker is always the first tag in the list.  The default value
    is "all".

  **-cap** *capStyle*

  **-coords**  *coordsList*
    Specifies the coordinates of the marker.  *CoordsList* is a list of
    graph coordinates.  The number of coordinates required is dependent on
    the type of marker.  Text, image, and window markers need only two
    coordinates (an X-Y coordinate).  Bitmap markers can take either two or
    four coordinates (if four, they represent the corners of the
    bitmap). Line markers need at least four coordinates, polygons at least
    six.  If *coordsList* is "", the marker will not be displayed.  The
    default is "".

  **-dashes**  *dashList*
    Sets the dash style of the outline of the polygon. *DashList* is a list
    of up to 11 numbers that alternately represent the lengths of the
    dashes and gaps on the outline.  Each number must be between 1
    and 255. If *dashList* is "", the outline will be a solid line.

  **-element**  *elemName*
    Links the visibility of *markerName with the *elemName*.  The marker is
    drawn only if the element is also currently displayed (see the
    element's **show** operation).  If *elemName* is "", the marker is
    always drawn.  The default is "".

  **-fill**  *colorName*
    Sets the fill color of the polygon.  If *colorName* is "", then the
    interior of the polygon is transparent.  The default is "white".

  **-hide**  *boolean* 
    Indicates whether the marker is drawn. If *boolean* is true, the marker
    is not drawn.  The default is "no".

  **-join**  *joinStyle* 

  **-linewidth**  *numPixels*
    Sets the width of the outline of the polygon. If *numPixels* is "0", no
    outline is drawn. The default is "0".

  **-mapx**  *axisName* 
    Specifies the X-axis to map the marker's X-coordinates onto.
    *AxisName* must the name of an axis.  The default is "x".

  **-mapy**  *axisName*
    Specifies the Y-axis to map the marker's Y-coordinates onto.
    *AxisName* must the name of an axis.  The default is "y".

  **-name**  *string*
    Changes the name for the marker.  The name *string* can not already be
    used by another marker.  If this option isn't specified, the marker's
    name is uniquely generated.

  **-outline**  *colorName*
    Sets the color of the outline of the polygon.  If the polygon is
    stippled (see the **-stipple** option), then this represents the
    foreground color of the stipple.  The default is "black".

  **-state**  *state*
    Specifies the state of the marker. *State* can be "normal" or "disabled".

  **-stipple**  *bitmapName*
    Specifies that the polygon should be drawn with a stippled pattern
    rather than a solid color. *BitmapName* specifies a bitmap to use as
    the stipple pattern.  If *bitmapName* is "", then the polygon is filled
    with a solid color (if the marker's **-fill** option is set).  The
    default is "".

  **-under**  *boolean*
    Indicates whether the marker is drawn below/above data elements.  If
    *boolean* is true, the marker is be drawn underneath the data element
    symbols and lines.  Otherwise, the marker is drawn on top of the
    element.  The default is "0".

  **-xoffset**  *numPixels*
    Specifies a screen distance to offset the marker horizontally.
    *NumPixels* is a valid screen distance, such as "2" or "1.2i".  The
    default is "0".

  **-xor**  *boolean*

  **-yoffset**  *numPixels*
    Specifies a screen distance to offset the markers vertically.
    *NumPixels* is a valid screen distance, such as "2" or "1.2i".  The
    default is "0".

*pathName* **marker create text** ?\ *option* *value* ... ?

  Creates a text marker. This command returns the marker name in the form
  "marker0","marker1", etc.  You can use the **-name** option to specify
  you own name for the marker.  

  Text marker configuration options may also be set by the **option**
  command.  The resource class is "TextMarker".  The resource name is the
  name of the marker.

    ::

      option add *Graph.TextMarker.fill blue

  There may be many *option*-*value* pairs, each sets a configuration
  option for the text marker.  These same *option*-*value* pairs may be
  used with the **marker configure** operation.

  The following options are specific to text markers:

  **-anchor**  *anchor*
    *Anchor* tells how to position the text relative to the positioning point
    for the text. For example, if *anchor* is "center" then the text is
    centered on the point; if *anchor* is "n" then the text will be drawn
    such that the top center point of the rectangular region occupied by the
    text will be at the positioning point.  This default is "center".

  **-background**  *color*
    Same as the **-fill** option.

  **-bindtags**  *tagList*
    Specifies the binding tags for the marker.  *TagList* is a list of
    binding tag names.  The tags and their order will determine how events
    for markers are handled.  Each tag in the list matching the current
    event sequence will have its TCL command executed.  Implicitly the name
    of the marker is always the first tag in the list.  The default value
    is "all".

  **-coords**  *coordsList*
    Specifies the coordinates of the marker.  *CoordsList* is a list of
    graph coordinates.  The number of coordinates required is dependent on
    the type of marker.  Text, image, and window markers need only two
    coordinates (an X-Y coordinate).  Bitmap markers can take either two or
    four coordinates (if four, they represent the corners of the
    bitmap). Line markers need at least four coordinates, polygons at least
    six.  If *coordsList* is "", the marker will not be displayed.  The
    default is "".

  **-element**  *elemName*
    Links the visibility of *markerName with the *elemName*.  The marker is
    drawn only if the element is also currently displayed (see the
    element's **show** operation).  If *elemName* is "", the marker is
    always drawn.  The default is "".

  **-fill**  *colorName*
    Specifies the background color of the text.  If *colorName* is the
    empty string, the background will be transparent.  The default
    background color is "".

  **-font**  *fontName*
    Specifies the font for the text.  The default is "{Sans Serif} 9".

  **-foreground**  *colorName*
    Same as the marker's **-outline** option.

  **-hide**  *boolean* 
    Indicates whether the marker is hidden. If *boolean* is true, the
    marker is not drawn.  The default is "no".

  **-justify**  *justifyName*
    Specifies how the text should be justified.  This matters only when the
    marker contains more than one line of text. *JustifyName* must be
    "left", "right", or "center".  The default is "center".

  **-mapx**  *axisName* 
    Specifies the X-axis to map the marker's X-coordinates onto.  *AxisName*
    must the name of an axis.  The default is "x".

  **-mapy**  *axisName*
    Specifies the Y-axis to map the marker's Y-coordinates onto.  *AxisName*
    must the name of an axis.  The default is "y".

  **-name**  *string*
    Changes the name for the marker.  The name *string* can not already be
    used by another marker.  If this option isn't specified, the marker's
    name is uniquely generated.

  **-outline**  *colorName*
    Specifies the color of the text. The default value is "black".

  **-padx**  *numPixels*
    Sets the padding to the left and right of the text.  *NumPixels* can be
    a list of one or two screen distances.  If *numPixels* has two
    elements, the left side of the text is padded by the first distance and
    the right side by the second.  If *numPixels* has just one distance,
    both the left and right sides are padded evenly.  The default is "4".

  **-pady**  *numPixels*
    Sets the padding above and below the text.  *NumPixels* can be a list
    of one or two screen distances.  If *numPixels* has two elements, the
    area above the text is padded by the first distance and the area below
    by the second.  If *numPixels* is just one distance, both the top and
    bottom areas are padded evenly.  The default is "4".

  **-rotate**  *numDegrees*
    Specifies the number of degrees to rotate the text.  *NumDegrees* is a
    real number representing the angle.  The marker will be rotated along
    its center and is drawn according to its anchor position. The default
    is "0.0".

  **-text**  *string*
    Specifies the text of the marker.  The exact way the text is displayed
    may be affected by other options such as **-anchor** or **-rotate**.

  **-under**  *boolean*
    Indicates whether the marker is to be drawn under elements.  If
    *boolean* is true, the marker is be drawn underneath the element
    symbols and lines.  Otherwise, the marker is drawn on top of the
    elements.  The default is "0".

  **-xoffset**  *numPixels*
    Specifies a screen distance to offset the marker horizontally.
    *NumPixels* is a valid screen distance, such as "2" or "1.2i".  The
    default is "0".

  **-yoffset**  *numPixels*
    Specifies a screen distance to offset the marker vertically.
    *NumPixels* is a valid screen distance, such as "2" or "1.2i".  The
    default is "0".

*pathName* **marker create window** ?\ *option* *value* ... ?

  Creates a window marker. This command returns the marker name in the form
  "marker0","marker1", etc.  You can use the **-name** option to specify
  you own name for the marker.

  Window marker configuration options may also be set by the **option**
  command.  The resource class is "WindowMarker".  The resource name is the
  name of the marker.

    ::

      option add *Graph.WindowMarker.anchor sw

  There may be many *option*-*value* pairs, each sets a configuration
  option for the marker.  These same *option*-*value* pairs may be used
  with the **marker configure** command.

  The following options are specific to window markers:

  **-anchor**  *anchor*
    *Anchor* tells how to position the marker relative its positioning
    point. For example, if *anchor* is "center" then the marker is centered
    on the point; if *anchor* is "n" then the marker will be displayed such
    that the top center point of the rectangular region occupied by the
    widget will be at the positioning point.  This option defaults to
    "center".

  **-bindtags**  *tagList*
    Specifies the binding tags for the marker.  *TagList* is a list of
    binding tag names.  The tags and their order will determine how events
    for markers are handled.  Each tag in the list matching the current
    event sequence will have its TCL command executed.  Implicitly the name
    of the marker is always the first tag in the list.  The default value
    is "all".

  **-coords**  *coordsList*
    Specifies the coordinates of the window marker.  *CoordsList* is a list
    of graph coordinates.  The number of coordinates required is dependent
    on the type of marker.  Text, image, and window markers need only two
    coordinates (an X-Y coordinate).  Bitmap markers can take either two or
    four coordinates (if four, they represent the corners of the
    bitmap). Line markers need at least four coordinates, polygons at least
    six.  If *coordsList* is "", the marker will not be displayed.  The
    default is "".

  **-element**  *elemName*
    Links the visibility of *markerName with the *elemName*.  The marker is
    drawn only if the element is also currently displayed (see the
    element's **show** operation).  If *elemName* is "", the marker is
    always drawn.  The default is "".

  **-height**  *numPixels*
    Specifies the height of the child window (see the **-window** option).
    If this option isn't specified, or if it is specified as "", then the
    window is given whatever height the widget requests internally.

  **-hide**  *boolean* 
    Indicates whether the to hide the marker. If *boolean* is true, the
    marker is not drawn.  The default is "no".

  **-mapx**  *axisName* 
    Specifies the X-axis to map the marker's X-coordinates onto.
    *AxisName* must the name of an axis.  The default is "x".

  **-mapy**  *axisName*
    Specifies the Y-axis to map the marker's Y-coordinates onto.
    *AxisName* must the name of an axis.  The default is "y".

  **-name**  *string*
    Changes the name for the marker.  The name *string* can not already be
    used by another marker.  If this option isn't specified, the marker's
    name is uniquely generated.

  **-state**  *state*
    Specifies the state of the marker. *State* can be "normal" or "disabled".
    
  **-under**  *boolean*
    Indicates whether the marker is drawn below/above data elements.  If
    *boolean* is true, the marker is be drawn underneath the data element
    symbols and lines.  Otherwise, the marker is drawn on top of the
    element.  The default is "0".

  **-width**  *numPixels*
    Specifies the width to assign to the child window (see the **-window**
    option).  If this option isn't specified, or if it is specified as "",
    then the window is given whatever width the widget requests internally.

  **-window**  *pathName*
    Specifies the widget to be managed by the graph.  *PathName* must be a
    child of the **blt::graph** widget.

  **-xoffset**  *numPixels*
    Specifies a screen distance to offset the marker horizontally.
    *NumPixels* is a valid screen distance, such as "2" or "1.2i".  The
    default is "0".

  **-yoffset**  *numPixels*
    Specifies a screen distance to offset the markers vertically.
    *NumPixels* is a valid screen distance, such as "2" or "1.2i".  The
    default is "0".

*pathName* **marker delete** ?\ *markerName* ... ?
  Removes one of more markers from *pathName*.  *MarkerName* is a marker
  name returned by the **marker create** operation.

*pathName* **marker exists**  *markerName* 
  Returns "1" if *markerName* exists and "0" otherwise.

*pathName* **marker find enclosed**  *x1* *y1* *x2* *y2*
  Finds all the markers that are completely enclosed within
  the  rectangular region given by *x1*, *y1*, *x2*, and *y2*.
  *X1*, *y1* and *x2*, *y2 are opposite corners of the region.

*pathName* **marker find overlapping**  *x1* *y1* *x2* *y2*
  Finds all the markers that overlap or are enclosed within
  the  rectangular region given by *x1*, *y1*, *x2*, and *y2*.
  *X1*, *y1* and *x2*, *y2 are opposite corners of the region.
  
*pathName* **marker get** *markerName*
  Converts the special marker specifier to it name.  This is
  used currently to convert "current" to the name of the currently
  picked marker.
  
*pathName* **marker lower** *markerName* ?\ *beforeName*\ ?
  Lowers *markerName* so that it will be drawn below other markers in
  *pathName*. If a *beforeName* argument is present, *markerName* will be
  positioned just below it.  Both *markerName* and *beforeName* are marker
  names. By default, markers are drawn in the order they were created.

*pathName* **marker names** ?\ *pattern* ... ?  
  Returns the names of all the markers in the graph.  If one or more
  *pattern* arguments are provided, then the name of any marker matching
  *pattern* will be returned. *Pattern* is a **glob**\ -style pattern.

*pathName* **marker raise** *markerName* ?\ *afterName*\ ?
  Raises *markerName* so that it will be drawn above other markers in
  *pathName*. If an *afterName* argument is present, *markerName* will be
  positioned just above it.  Both *markerName* and *afterName* are marker
  names. By default, markers are drawn in the order they were created.

*pathName* **marker type** *markerName* 
  Returns the type of *markerName* (for example "image").  *MarkerName* is
  the name of a marker. If *markerName* isn't valid, "" is returned.


COMPONENT BINDINGS
------------------

Specific graph components, such as elements, markers and legend entries,
can have a command trigger when event occurs in them, much like canvas
items in Tk's canvas widget.  Not all event sequences are valid.  The only
binding events that may be specified are those related to the mouse and
keyboard (such as **Enter**, **Leave**, **ButtonPress**, **Motion**, and
**KeyPress**).

Only one element or marker can be picked during an event.  This means,
that if the mouse is directly over both an element and a marker, only
the uppermost component is selected.  This isn't true for legend entries.  
Both a legend entry and an element (or marker) binding commands 
will be invoked if both items are picked.

It is possible for multiple bindings to match a particular event.  This
could occur, for example, if one binding is associated with the element
name and another is associated with one of the element's tags (see the
**-bindtags** option).  When this occurs, all of the matching bindings are
invoked.  A binding associated with the element name is invoked first,
followed by one binding for each of the element's bindtags.  If there are
multiple matching bindings for a single tag, then only the most specific
binding is invoked.  A continue command in a binding script terminates that
script, and a break command terminates that script and skips any remaining
scripts for the event, just as for the **bind** command.

The **-bindtags** option for these components controls addition tag names
which can be matched.  Implicitly elements and markers always have tags
matching their names.  Setting the value of the **-bindtags** option
doesn't change this.

EXAMPLE
-------

The **blt::graph** command creates a new graph.  

 ::

    # Create a new graph.  Plotting area is black.

    blt::graph .g -plotbackground black

A new TCL command ".g" is also created.  This command can be used to query
and modify the graph.  For example, to change the title of the graph to "My
Plot", you use the new command and the graph's **configure** operation.

 ::

    # Change the title.

    .g configure -title "My Plot"

A graph has several components. To access a particular component you use
the component's name. For example, to add data elements, you use the new
command and the **element create** operation.

 ::

    # Create a new element named "line1"

    .g element create line1 \
            -xdata { 0.2 0.4 0.6 0.8 1.0 1.2 1.4 1.6 1.8 2.0 } \
            -ydata { 26.18 50.46 72.85 93.31 111.86 128.47 143.14 
                    155.85 166.60 175.38 }

The element's X-Y coordinates are specified using lists of numbers.
Alternately, BLT vectors could be used to hold the X-Y coordinates.

 ::

    # Create two vectors and add them to the graph.

    blt::vector xVec yVec
    xVec set { 0.2 0.4 0.6 0.8 1.0 1.2 1.4 1.6 1.8 2.0 }
    yVec set { 26.18 50.46 72.85 93.31 111.86 128.47 143.14 155.85 
            166.60 175.38 }
    .g element create line1 -xdata xVec -ydata yVec

The advantage of using vectors is that when you modify one, the graph is
automatically redrawn to reflect the new values.

 ::

    # Change the y coordinate of the first point.

    set yVector(0) 25.18

An element named "e1" is now created in ".b".  It is automatically added to
the display list of elements.  You can use this list to control in what
order elements are displayed.  To query or reset the element display list,
you use the element's **show** operation.

 ::

    # Get the current display list 

    set elemList [.b element show]

    # Remove the first element so it won't be displayed.

    .b element show [lrange $elemList 0 end]

The element will be displayed by as many bars as there are data points (in
this case there are ten).  The bars will be drawn centered at the
x-coordinate of the data point.  All the bars will have the same attributes
(colors, stipple, etc).  The width of each bar is by default one unit.  You
can change this with using the **-barwidth** option.

 ::

    # Change the X-Y coordinates of the first point.

    set xVec(0) 0.18
    set yVec(0) 25.18

An element named "line1" is now created in ".g".  By
default, the element's label in the legend will be also "line1".
You can change the label, or specify no legend entry, again using the
element's **configure** operation.

 ::

    # Don't display "line1" in the legend.

    .g element configure line1 -label ""

You can configure more than just the element's label.  An element has many
attributes such as symbol type and size, dashed or solid lines, colors,
line width, etc.

 ::

    .g element configure line1 -symbol square -color red \
        -dashes { 2 4 2 } -linewidth 2 -pixels 2c

Four coordinate axes are automatically created: "x", "x2", "y", and "y2".
And by default, elements are mapped onto the axes "x" and "y".  This can be
changed with the **-mapx** and **-mapy** options.

 ::

    # Map "line1" on the alternate Y-axis "y2".

    .g element configure line1 -mapy y2

Axes can be configured in many ways too.  For example, you change the scale
of the Y-axis from linear to log using the **axis configure** operation.

 ::

    # Y-axis is log scale.

    .g axis configure y -logscale yes

One important way axes are used is to zoom in on a particular data region.
Zooming is done by simply specifying new axis limits using the **-min** and
**-max** configuration options.

 ::

    .g axis configure x -min 1.0 -max 1.5
    .g axis configure y -min 12.0 -max 55.15

To zoom interactively, you link the **axis configure** operations with some
user interaction (such as pressing the mouse button), using the **bind**
command.  To convert between screen and graph coordinates, use the
**invtransform** operation.

 ::

    # Click the button to set a new minimum 

    bind .g <ButtonPress-1> { 
        %W axis configure x -min [%W axis invtransform x %x]
        %W axis configure x -min [%W axis invtransform x %y]
    }

By default, the limits of the axis are determined from data values.  To
reset back to the default limits, set the **-min** and **-max** options to
the empty value.

 ::

    # Reset the axes to autoscale again.

    .g axis configure x -min {} -max {}
    .g axis configure y -min {} -max {}

By default, the legend is drawn in the right margin.  You can change this
or any legend configuration options using the **legend configure**
operation.

 ::

    # Configure the legend font, color, and relief

    .g legend configure -position left -relief raised \
        -font fixed -fg blue

To prevent the legend from being displayed, turn on the **-hide**
option.

 ::

    # Don't display the legend.

    .g legend configure -hide yes

The **blt::graph** widget has simple drawing procedures called markers.
They can be used to highlight or annotate data in the graph. The types
of markers available are bitmaps, images, polygons, lines, or windows.
Markers can be used, for example, to mark or brush points.  In this
example, is a text marker that labels the data first point.  Markers
are created using the **marker create** operation.

 ::

    # Create a label for the first data point of "line1".

    .g marker create text -name first_marker -coords { 0.2 26.18 } \
        -text "start" -anchor se -xoffset -10 -yoffset -10

This creates a text marker named "first_marker".  It will display
the text "start" near the coordinates of the first data point.  The
**-anchor**, **-xoffset**, and **-yoffset** options are used
to display the marker above and to the left of the data point, so that
the data point isn't covered by the marker.  By default,
markers are drawn last, on top of data.  You can change this with the
**-under** option.

 ::

    # Draw the label before elements are drawn.

    .g marker configure first_marker -under yes

You can add cross hairs or grid lines using the **crosshairs** and
**axis** operations.

 ::

    # Display both cross hairs and grid lines on the X axis.

    .g crosshairs configure -hide no -color red
    .g axis configure x -gridlines yes -griddashes { 2 2 }

    # Set up a binding to reposition the crosshairs.

    bind .g <Motion> {
        .g crosshairs configure -position @%x,%y
    }

The crosshairs are repositioned as the mouse pointer is moved
in the graph.  The pointer X-Y coordinates define the center
of the crosshairs.

Finally, to get hardcopy of the graph, use the **postscript output**
operation.

 ::

    # Print the graph into file "file.ps"

    .g postscript output file.ps -maxpect yes -decorations no

This generates a file "file.ps" containing the encapsulated PostScript of
the graph.  The option **-maxpect** says to scale the plot to the size of
the page.  Turning off the **-decorations** option denotes that no borders
or color backgrounds should be drawn (i.e. the background of the margins,
legend, and plotting area will be white).


C LANGUAGE API
--------------

You can manipulate data elements from the C language.  There may be
situations where it is too expensive to translate the data values from
ASCII strings.  Or you might want to read data in a special file format.

Data can manipulated from the C language using BLT vectors.  You specify
the X-Y data coordinates of an element as vectors and manipulate the vector
from C.  The graph will be redrawn automatically after the vectors are
updated.

From TCL, create the vectors and configure the element to use them.

::

    vector X Y
    .g element configure line1 -xdata X -ydata Y

To set data points from C, you pass the values as arrays of doubles using
the **Blt_ResetVector** call.  The vector is reset with the new data and at
the next idle point (when Tk re-enters its event loop), the graph will be
redrawn automatically.

 ::

    #include <tcl.h>
    #include <blt.h>

    register int i;
    Blt_Vector *xVec, *yVec;
    double x[50], y[50];

    /* Get the BLT vectors "X" and "Y" (created above from TCL) */
    if ((Blt_GetVector(interp, "X", &xVec) != TCL_OK) ||
        (Blt_GetVector(interp, "Y", &yVec) != TCL_OK)) {
        return TCL_ERROR;
    }

    for (i = 0; i < 50; i++) {
        x[i] = i * 0.02;
        y[i] = sin(x[i]);
    }   

    /* Put the data into BLT vectors */
    if ((Blt_ResetVector(xVec, x, 50, 50, TCL_VOLATILE) != TCL_OK) ||
        (Blt_ResetVector(yVec, y, 50, 50, TCL_VOLATILE) != TCL_OK)) {
       return TCL_ERROR;
    }

See the **blt::vector** manual page for more details.

SPEED TIPS
----------

There may be cases where the graph needs to be drawn and updated as quickly
as possible.  If drawing speed becomes a big problem, here are a few tips
to speed up displays.

 1. Try to minimize the number of data points.  The more data points the
    looked at, the more work the graph must do.

 2. If your data is generated as floating point values, the time required
    to convert the data values to and from ASCII strings can be
    significant, especially when there any many data points.  You can avoid
    the redundant string-to-decimal conversions using the C API to BLT
    vectors.

 3. Data elements without symbols are drawn faster than with symbols.  Set
    the data element's **-symbol** option to "none".  If you need to draw
    symbols, try using the simple symbols such as "splus" and "scross".

 4. Don't stipple or dash the element.  Solid lines are much faster.

 5. If you update data elements frequently, try turning off the widget's
    **-bufferelements** option.  When the graph is first displayed, it
    draws data elements into an internal pixmap.  The pixmap acts as a
    cache, so that when the graph needs to be redrawn again, and the data
    elements or coordinate axes haven't changed, the pixmap is simply
    copied to the screen.  This is especially useful when you are using
    markers to highlight points and regions on the graph.  But if the graph
    is updated frequently, changing either the element data or coordinate
    axes, the buffering becomes redundant.

LIMITATIONS
-----------

Auto-scale routines do not use requested min/max limits as boundaries when
the axis is logarithmically scaled.

The PostScript output generated for polygons with more than 1500 points may
exceed the limits of some printers (See PostScript Language Reference
Manual, page 568).  The work-around is to break the polygon into separate
pieces.

DIFFERENCES WITH PREVIOUS VERSIONS
----------------------------------

1. There no longer a grid component.  Grid settings are part of the
   the axis now.
2. There is no longer a **-tile** option for the graph. The **-background**
   option with a **blt::background** does this now.
3. There is no longer an **-areatile** element option. The blt::background
   does this.
4. There is no longer an **-areapattern** element option. The
   blt::background does this.
5. There is no longer a "@bitmap" symbol type.  There are images now.
6. The axis, element, marker, and pen configure operations no longer take
   multiple items as arguments. Elements have tags now.
7. The axis **-limits** axis option is replaced by the **-limitsformat**
   option.
8. There is no longer a **-maxpect**  postscript option.



KEYWORDS
--------

graph, widget

COPYRIGHT
---------

2015 George A. Howlett. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 1) Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2) Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the distribution.
 3) Neither the name of the authors nor the names of its contributors may
    be used to endorse or promote products derived from this software
    without specific prior written permission.
 4) Products derived from this software may not be called "BLT" nor may
    "BLT" appear in their names without specific prior written permission
    from the author.

THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
