#!/bin/sh
SRC=`basename $1`
DEST=$2
cp $1 $SRC
rm -f $DEST
rst2man.py $SRC > $DEST
rm -f $SRC
