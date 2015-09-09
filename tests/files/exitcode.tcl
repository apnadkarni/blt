if { [llength $argv] != 1 } {
  puts stderr "usage: exitcode number"
  exit 1
}
set code [lindex $argv 0]
if { ![string is integer $code] } {
  puts stderr "bad exist code \"$code\""
  exit 2
}
exit $code
