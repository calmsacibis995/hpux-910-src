#! /bin/sh
# @(#) $Revision: 64.1 $
#
#	INSTALL COMMAND

USAGE="eval echo 'usage: install [options] file [dir1 ...]'; exit 2"
PATH=/bin:/etc:/usr/bin
FLIST=$ROOT/etc/syslist
DEFAULT="$ROOT/bin $ROOT/usr/bin $ROOT/etc $ROOT/lib $ROOT/usr/lib"
ECHO=echo
FLAG=off
FOUND=""
MOVOLD=""
LASTRES=""

MODE=755
GROUP=`id -gn`
OWNER=`id -un`

if [ "$OWNER" = root ]; then
    ROOTFLAG=on
    OWNER=bin
    GROUP=bin
else
    ROOTFLAG=off
fi

for i in $*; do
    if [ "$FLAG" = on ]; then
	case $i in
	-*)
	    echo "install: The -c, -f, -n options each require a directory following!"
	    exit 2
	    ;;
	 *)
	    FLAG=off
	    continue
	    ;;
	esac
    fi

    case $i in
    -c)
	if [ x$ARG = x-f -o x$arg = x-i -o x$arg = x-o -o x$arg = x-n ]; then
	    echo "install: -c dir: illegal option with ${arg-"-f"} option!"
	    exit 2
	elif [ $# -lt 3 ]; then
	    echo "install: -c option must have at least 3 args!"
	    exit 2
	else
	    direct=$2
	    FLAG=on
	    ARG=-c
	    shift 2
	fi
	;;
    -f)
	if [ x$ARG = x-c -o x$arg = x-i -o x$arg = x-n ]; then
	    echo "install: -f dir: illegal option with ${arg-"-c"} option!"
	    exit 2
	elif [ $# -lt 3 ]; then
	    echo "install: -f option must have at least 3 args!"
	    exit 2
	else
	    direct=$2
	    FLAG=on
	    ARG=-f
	    shift 2
	fi
	;;
    -i)
	if [ x$ARG  = x-c -o x$ARG = x-f ]; then
	    echo "install: -i: illegal option with $ARG option!"
	    exit 2
	elif [ $# -lt 3 ]; then
	    echo "install: -i option requires at least 3 args!"
	    exit 2
	else
	    DEFAULT=""
	    arg=-i
	    shift
	fi
	;;
    -o)
	if [ x$ARG = x-c ]; then
	    echo "install: -o: illegal option with $ARG option!"
	    exit 2
	elif [ $# -lt 2 ]; then
	    $USAGE
	else
	    MOVOLD=yes
	    arg=-o
	    shift
	fi
	;;
    -n) if [ x$ARG = x-c -o x$ARG = x-f ]; then
	    echo "install: -n dir: illegal option with $ARG option!"
	    exit 2
	elif [ $# -lt 3 ]; then
	    echo "install: -n option requires at least 3 args!"
	    exit 2
	else
	    LASTRES=$2
	    FLAG=on
	    FOUND=n
	    arg=-n
	    shift 2
	fi
	;;
    -s)
	if [ $# -lt 2 ]; then
	    $USAGE
	else
	    ECHO=:
	    arg=-s
	    shift
	fi
	;;
    -u)
	if [ "$ROOTFLAG" = off ]; then
	    echo "install: -u option available only to root -- ignored"
	else
	    OWNER=$2
	    echo new owner is $OWNER
	fi
	FLAG=on
	shift 2
	;;
    -g)
	if [ "$ROOTFLAG" = off ]; then
	    echo "install: -g option available only to root -- ignored"
	else
	    GROUP=$2
	fi
	FLAG=on
	shift 2
	;;
    -m)
	MODE=$2
	FLAG=on
	shift 2
	;;
     *) 
	break
	;;
    esac
done

FILEP=$i FILE=`echo $i | sed -e "s/.*\///"`
if [ x$ARG = x-c -o x$ARG = x-f ]; then
    case $2 in
    -*)
	$USAGE
	;;
    "")
	;;
    esac

    if [ -f $direct/$FILE ]; then
	case $ARG in
	-c)
	    echo "install: $FILE already exists in $direct"
	    exit 2
	    ;;
	-f)
	    if [ -k $direct/$FILE ]; then
		chmod -t $direct/$FILE
		$direct/$FILE < /dev/null > /dev/null
		tbit=on
	    fi
	    if [ "$MOVOLD" = yes ]; then
		mv $direct/$FILE $direct/OLD$FILE
		if cp $direct/OLD$FILE $direct/$FILE; then
		   $ECHO "$FILE moved to $direct/OLD$FILE"
		   chgrp $GROUP $direct/$FILE
		else
		   echo "install: cp $direct/OLD$FILE $direct/$FILE failed"
		   exit 2
		fi
	    fi
	    if cp $FILEP $direct/$FILE; then
		$ECHO "$FILEP installed as $direct/$FILE"
	    fi
	    if [ "$tbit" = on ]; then
		chmod +t $direct/$FILE
	    fi
	    exit 0
	;;
	esac
    else
	if cp $FILEP $direct/$FILE; then
	    $ECHO "$FILEP installed as $direct/$FILE"
	    chgrp $GROUP $direct/$FILE
	    chown $OWNER $direct/$FILE
	    chmod $MODE $direct/$FILE
	fi
    fi
    exit 0
fi

if [ $# -eq 0 ]; then
    $USAGE
fi
shift

PUTHERE=""
for i in $*; do
    case $i in
    -*)
	$USAGE
	;;
    esac

    PUTHOLD=`find $i -name $FILE -type f -print`
    PUTHERE=`expr "\`echo $PUTHOLD\`" : '\([^ ]*\)'`
    if [ "$PUTHERE" != "" ]; then
	break
    fi
done

if [ -r $FLIST -a "$PUTHERE" = "" ]; then
    PUTHERE=$ROOT`grep "/${FILE}$" $FLIST | sed  -n -e '1p'`
fi

if [ "$DEFAULT" = "" -a "$LASTRES" = "" ]; then
    PUTHERE=$i
fi

if [ "$PUTHERE" = "" ]; then
    for i in $DEFAULT; do
	PUTHOLD=`find $i -name $FILE -type f -print`
	PUTHERE=`expr "\`echo $PUTHOLD\`" : '\([^ ]*\)'`
	if [ "$PUTHERE" != "" ]; then
	    break
	fi
    done
fi

if [ "$PUTHERE" != "" ]; then
    if [ -k $PUTHERE ]; then
	chmod -t $PUTHERE
	$PUTHERE < /dev/null > /dev/null
	tbit=on
    fi
    if [ "$MOVOLD" = yes ]; then
	old=`echo $PUTHERE | sed -e "s/\/[^\/]*$//"`
	mv $PUTHERE $old/OLD$FILE
	if cp $old/OLD$FILE $PUTHERE; then
	    $ECHO "old $FILE moved to $old/OLD$FILE"
	    chgrp $GROUP $PUTHERE
	else
	   echo "install: cp $direct/OLD$FILE $direct/$FILE failed"
	    exit 2
	fi
    fi
    FOUND=y
    if cp $FILEP $PUTHERE; then
	if [ "$tbit" = on ]; then
	    chmod +t $PUTHERE
	fi
	$ECHO "$FILEP installed as $PUTHERE"
	break
    else
	exit 2
    fi
fi

case $FOUND in
"")
    echo "install: $FILE was not found anywhere!"
    exit 2
    ;;
y)
    ;;
n)
    if cp $FILEP $LASTRES/$FILE; then
	$ECHO "$FILEP installed as $LASTRES/$FILE by default!"
	cd $LASTRES
	chgrp $GROUP $FILE
	chown $OWNER $FILE
	chmod $MODE $FILE
    fi
    ;;
esac
