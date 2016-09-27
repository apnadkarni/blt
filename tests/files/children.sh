#!/bin/sh
echo "starting 1st process" >> /dev/tty
/bin/sh ./files/grandchild.sh &
echo "starting 2nd process" >> /dev/tty
/bin/sh ./files/grandchild.sh &
sleep 5
echo "Should not see this" 
exit 1
