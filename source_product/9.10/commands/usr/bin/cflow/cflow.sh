#! /bin/sh
# @(#) $Revision: 70.3.1.1 $      
INVFLG=
DFLAG=
IFLAG=
#ifdef OSF
#ifdef PAXDEV
DIR=/paXdev/lib
CC=/paXdev/bin/cc
LINT1=/paXdev/lib/lint1
#else
DIR=/usr/ccs/lib
CC=/usr/ccs/bin/c89
LINT1=/usr/ccs/lib/lint1
#endif
#else
DIR=/usr/lib
CC=/bin/cc
LINT1=/usr/lib/lint1
#endif

#ifdef OSF
if [ "$TMPDIR" = "" ]; then
   TMPDIR=/tmp
fi
#else
TMPDIR=/usr/tmp
#endif

TMP=$TMPDIR/cf.$$
TMPG=$TMP.g

#if defined(OSF) && !defined(PAXDEV)
l="-A"
#endif

trap "rm -f $TMP.?; kill $$" 1 2 3
echo >$TMP.g

while [ "$1" != "" ]
do
	case "$1" in
	-r)
		INVFLG=1
		;;
	-d*)
		DFLAG=$1
		;;
	-i*)
		IFLAG="$IFLAG $1"
		;;
	-f)
		cat $2 </dev/null >>$TMPG
		shift
		;;
	-g)
		TMPG=$2
		if [ "$TMPG" = "" ]
		then
			TMPG=$TMP.g
		fi
		shift
		;;
	-[IDU]*)
		o="$o $1"
		;;
	-N*)
		l="$l $1"
		;;
	-Aa)
		l="$l -A"
		o="$o -Aa"
		;;
	-Ac)
		o="$o -Ac"
		;;
	-Y)
		o="$o $1"
		l="$l -n"
		;;
	*.y)
		yacc $1
		sed -e "/^# line/d" y.tab.c > $1.c
		$CC -E $o $1.c | $LINT1 $l -H$TMP.j $1.c\
			| $DIR/lpfx $IFLAG >>$TMPG
		rm y.tab.c $1.c
		;;
	*.l)
		lex $1
		sed -e "/^# line/d" lex.yy.c > $1.c
		$CC -E $o $1.c | $LINT1 $l -H$TMP.j $1.c\
			| $DIR/lpfx $IFLAG >>$TMPG
		rm lex.yy.c $1.c
		;;
	*.c)
		$CC -E $o $1 | $LINT1 $l -H$TMP.j $1\
			| $DIR/lpfx $IFLAG >>$TMPG
		;;
	*.i)
		name=`basename $1 .c`
		$LINT1 $l -H$TMP.j <$1 | $DIR/lpfx >>$TMPG $name.c
		;;
	*.s)
		a=`basename $1 .s`
                if hp9000s800
		  then           # 800 nm diff from others, so do in diff way
		    cc -c $1
		    nm -hen $a.o | $DIR/nmf $a ${a}.s >>$TMPG
		  else
		    as -o $TMP.o $1
		    nm -gn $TMP.o | $DIR/nmf $a ${a}.s >>$TMPG
                fi
		;;
	*.o)
		a=`basename $1 .o`
		if hp9000s800
		  then		# 800 nm diff from others, so do in diff way
		    nm -hen $1 | $DIR/nmf $a ${a}.o >>$TMPG
		  else
		    nm -gn $1 | $DIR/nmf $a ${a}.o >>$TMPG
		fi
		;;
	*)
		echo $1 "-- cflow can't process - file skipped"
		;;
	esac
	shift
done
if [ "$INVFLG" != "" ]
then
	grep "=" $TMPG >$TMP.q
	grep ":" $TMPG | $DIR/flip >>$TMP.q
	sort <$TMP.q >$TMPG
	rm $TMP.q
fi
$DIR/dag $DFLAG <$TMPG
rm -f $TMP.?
