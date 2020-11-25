#!/bin/sh

#assumes simulator in current directory

TMPDIR=$HOME/tmp
mkdir -p $TMPDIR

PRG=./y86-sim

for f in "$@"
do
  echo "testing $f"
  gold=`echo $f | sed -e 's/\.ys$/.out/'`

    if [ -e $gold ]
    then
    	tmp=$TMPDIR/$(basename $gold)
	if echo $f | grep -q 'main' 
	then
	    $PRG $f `seq 1 10` > $tmp
	else
	    $PRG $f > $tmp
	fi
    	if [ $? -eq 0 ]
    	then
    	    if diff $gold $tmp
    	    then
    		rm -f $tmp
    	    else
    		echo "*** $f failed; see output in $tmp"
    	    fi
    	else
    	    echo "cannot run \"$PRG $f > $tmp\""
    	fi
    else
    	echo "no out file for $f"
    fi
done
