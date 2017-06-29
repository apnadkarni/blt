
proc ContinueColumnMove { tv x1 y1 x2 y2 } {
  set col [$tv column find $x1 $y1 $x2 $y2]
  set token [.dnd cget -token]
  if  { $col == $token } {
    return "-1"
  }
  if { $col == -1 } {
    $tv column move deactivate $token
  } else {
    $tv column move activate $token $col
  }
  return $col
}

proc FinishColumnMove { tv colName } {
  set col [$tv column index $colName]
  if { $col == -1 } {
    $tv column move cancel $token
    .dnd cancel 
  } else {
    $tv column move finish $token $col
    .dnd forget 
  }
}

proc StartColumnMove { tv column } {
  blt::dnd .dnd -icon $icon -image $image \
      -motioncommand [list ContinueColumnMove $tv] \
      -dropcommand [list FinishColumnMove $tv] \
      -token $column
  .dnd start
} 