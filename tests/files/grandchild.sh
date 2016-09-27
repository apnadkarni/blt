#!/bin/sh
trap 'echo killed >> /dev/tty; exit 0' INT HUP TERM
sleep 20
echo "not killed"
exit 1
