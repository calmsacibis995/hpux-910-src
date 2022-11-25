#!/bin/sh
:
# @(#) $Revision: 66.4 $    
MERGE=no
KEEP_OLD=no
LIBDIR=/usr/lib
TEMPZ=/tmp/whatiscompr
DEF_PATH="/usr/man /usr/contrib/man /usr/local/man "  # space at end importnt


#  Check for -m option
if [ "$1" = "-m" ]
then
    MERGE=yes
    shift
fi

# check for directory argument, if not present, check for MANPATH.
# if MANPATH set, call parseman and set dirs to the result.
# If none of above true, use default set of paths.
if [ "$1" ]
then
    dirs=$1
    MERGE=yes
elif [ "$MANPATH" ]
then
    dirs=`$LIBDIR/parseman $MANPATH`
    if [ "$dirs" != "$DEF_PATH" -a "$MERGE" = no ]
    then
	KEEP_OLD=yes
    fi
else
    dirs="$DEF_PATH"
fi

for d in $dirs
do
	if [ -d $d ]
	then
 		for i in $d/man[1-9]*
	    	do
			if [ -d $i ]
			then
				cd $i
				case $i in
					*.Z) find $i -type f -print 2> /dev/null | while read FILE
					     do
						uncompress -c < $FILE > $TEMPZ
						$LIBDIR/getNAME $TEMPZ
					     done;;
					*)   $LIBDIR/getNAME *.*;;
				esac
			fi
		done 
	fi
done |\
sed -e 's/  */ /g' \
    -e 's/[ 	]-[ 	]/ \\- /' \
    -e 's/[ 	]\\-[ 	]/ \\- /' \
    -e 's/[ 	]\\(em[ 	]/ \\- /' \
    -e 's/[ 	]\\(mi[ 	]/ \\- /' \
    -e 's/^\.TH [^ ]* \([^ 	]*\).*	\(.*\) \\- /\2(\1)	- /' |\
deroff | tr -s ' ' ' ' |\
expand -24,28,32,36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96,100 |\
sort -u | unexpand -a >/tmp/whatis$$ 

#  If MERGE, sort the old whatis and the new one together.
#  Otherwise, just replace the old one with the new one.
if [ "$MERGE" = yes ] 
then
    if [ -f $LIBDIR/whatis ]
    then
        sort -u /tmp/whatis$$ $LIBDIR/whatis >/tmp/whatis.m$$
    else
	sort -u /tmp/whatis$$ >/tmp/whatis.m$$
    fi
    mv /tmp/whatis.m$$ $LIBDIR/whatis
else
    if [ "$KEEP_OLD" = yes ]
    then
	mv $LIBDIR/whatis $LIBDIR/whatis.old
    fi
    sort -u /tmp/whatis$$ >$LIBDIR/whatis
fi
chmod 644 $LIBDIR/whatis
rm -f $TEMPZ /tmp/whatis$$ /tmp/whatis.m$$
