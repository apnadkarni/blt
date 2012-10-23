package require BLT
set img [image create picture]
$img import ps -file sc.ps -dpi 600
$img export png -file sc2.png
