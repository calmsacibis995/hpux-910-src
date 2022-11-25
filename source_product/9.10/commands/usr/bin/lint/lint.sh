#! /bin/sh
# @(#) B.08.00 LANGUAGE TOOLS Internal $Revision: 70.9.1.2 $
#
# New lint shell script.  Changed to make lint(1) act as much as is possible
# like a different version of the cc(1) command.  This includes the notion of
# a ``lint .o'' (.ln) and incremental linting. 
#
TMPDIR=${TMPDIR:-/usr/tmp}
TOUT=$TMPDIR/tlint.$$		# combined input for second pass
HOUT=$TMPDIR/hlint.$$		# header messages file
#ifdef OSF
#ifdef PAXDEV
LDIR=/paXdev/lib		# where first & second pass are
LLDIR=/paXdev/lib		# where lint libraries are found
PATH=/bin:/usr/bin
CC_CMD=cc
#else
LDIR=/usr/ccs/lib		# where first & second pass are
LLDIR=/usr/ccs/lib/lint		# where lint libraries are found
PATH=/bin:/usr/bin:/usr/ccs/bin
CC_CMD=c89
#endif /* PAXDEV */
#else
LDIR=/usr/lib			# where first & second pass are
LLDIR=/usr/lib			# where lint libraries are found
PATH=/bin:/usr/bin
CC_CMD=cc
#endif /* OSF */
CCF="-E -C -D__lint -D__LINT__"	# options for the cc command
#ifdef OSF
LINTF="-A"			# options for the lint passes
#else
LINTF=				# options for the lint passes
#endif /* OSF */
LINT2F=				# options for lint2
FILES=				# the *.c and *.ln files in order
NDOTC=				# how many *.c were there
#ifdef OSF
DEFL=$LLDIR/llib-lc.ln
#else
DEFL=$LLDIR/llib-lc.ln		# the default library to use
#endif /* OSF */
LLIB=				# lint library file to create
CONLY=				# set for ``compile only''
pre=				# these three variables used for
post=				# handling options with arguments
optarg=				# list variable to add argument to
MODE=				# ANSI vs compatibility
NAMESPACE=			# e.g. -D_POSIX_SOURCE causes llib-lc relinting
LFN_SIZE=0                      # LFN file system support.
LFN_FNAME=                      # temporary.


#
trap "rm -f $TOUT $HOUT; exit 2" 1 2 3 15
#
# First, run through all of the arguments, building lists
#
#	lint's options are "A:abchl:no:psuvxw:M:" 
#	cc/cpp options are "W:I:D:U:gO"
#
if [ $# -eq 0 ]
then
	echo "usage: lint -[abhuvxnpscDUI] [-MA] [-A{ac}] [-l<lib>] [-c|-o<lib>][-w <class>] files"
	exit 0
fi
for OPT in "$@"
do
	if [ "$optarg" ]
	then
		if [ "$optarg" = "LLIB" ]	# special case...
		then
			OPT=`basename $OPT`
		fi
		eval "$optarg=\"\$$optarg \$pre\$OPT\$post\""
		pre=
		post=
		optarg=
		continue
	fi
	case "$OPT" in
	*.c)	FILES="$FILES $OPT"	NDOTC="x$NDOTC";;
	*.ln)	FILES="$FILES $OPT";;
	+*)	;;
	-*)	OPT=`echo $OPT | sed s/-//p`
		while [ "$OPT" ]
		do
			O=`echo $OPT | sed 's/\\(.\\).*/\\1/p'`
			OPT=`echo $OPT | sed s/.//p`
			case $O in
			p)	LINTF="$LINTF -p"
				CCF="$CCF -Wp,-T"
				DEFL=$LLDIR/llib-port.ln;;
			n)	DEFL=;;
			c)	CONLY=1;;
			[abhsuvxy]) LINTF="$LINTF -$O";;
			[gO])	CCF="$CCF -$O";;
			Y)	LINTF="$LINTF -n";;
			Z)	LINT2F="$LINT2F -$O$OPT"
				break;;
			N)	LINTF="$LINTF -$O$OPT"
				break;;
			M)	if [ "$O$OPT" = "MA" ]
				then
	       				LINTF="$LINTF -A"
					MODE=ANSI
					DEFL=$LLDIR/llib-lansi.ln
				fi
				break;;
			w)	if [ "$OPT" ]
				then
					LINTF="$LINTF -w$OPT"
				fi
				break ;;
			[AWIDU]) if [ "$OPT" ]
				then
					CCF="$CCF -$O$OPT"
				else
					optarg=CCF
					pre=-$O
				fi
				if [ "$O$OPT" = "Aa" ]
				then
					LINTF="$LINTF -A"
					MODE=ANSI
				fi

				# Need to re-lint llib-lc if a namespace 
				# variable is set; e.g. -D_POSIX_SOURCE
				DEF="$O$OPT"
				if [ \( "$DEF" = "D__STDC__" \) -o \
				     \( "$DEF" = "D_POSIX_SOURCE" \) -o \
				     \( "$DEF" = "D_POSIX2_SOURCE" \) -o \
				     \( "$DEF" = "D_XOPEN_SOURCE" \) -o \
				     \( "$DEF" = "D_CLASSIC_TYPES" \) -o \
				     \( "$DEF" = "D_CLASSIC_ANSI_TYPES" \) -o \
				     \( "$DEF" = "D_CLASSIC_POSIX_TYPES" \) -o \
				     \( "$DEF" = "D_CLASSIC_XOPEN_TYPES" \) -o \
				     \( "$DEF" = "D_CLASSIC_ID_TYPES" \) ]
				then
					NAMESPACE=NONHPUX
				fi
				break;;
			l)	if [ "$OPT" ]
                                then
                                    LFN_SIZE=`echo "$OPT\c" | wc -c`
                                    if [ $LFN_SIZE -gt 5 ]
                                    then
                                      LFN_FNAME=llib-l$OPT.ln
                                      if [ -r $LLDIR/$LFN_FNAME ]
                                      then
                                        FILES="$FILES $LLDIR/llib-l$OPT.ln"
                                      else
                                        LFN_FNAME=`expr substr $LFN_FNAME 0 14`
                                        if [ -r $LLDIR/$LFN_FNAME ]
                                        then
                                          FILES="$FILES $LLDIR/$LFN_FNAME"
                                        else
                                          FILES="$FILES $LLDIR/llib-l$OPT.ln"
                                        fi
                                      fi
                                    else
                                      FILES="$FILES $LLDIR/llib-l$OPT.ln"
                                    fi
                                else
                                        optarg=FILES
                                        pre=$LLDIR/llib-l
                                        post=.ln
				fi
				break;;
			o)	if [ "$OPT" ]
				then
					OPT=`basename $OPT`
					LLIB="llib-l$OPT.ln"
				else
					LLIB=
					optarg=LLIB
					pre=llib-l
					post=.ln
				fi
				break;;
			*)	echo "lint: bad option ignored: $O";;
			esac
		done;;
	*)	echo "lint: file with unknown suffix ignored: $OPT";;
	esac
done
#
# Second, walk through the FILES list, running all .c's through
# lint's first pass, and just adding all .ln's to the running result
#
if [ "$MODE" != "ANSI" ]  # avoid polluting ANSI namespace
then
	CCF="$CCF -Dlint"
else
         CCF=`echo $CCF |  sed s/-Wp,-T//p`
fi
if [ "$NDOTC" != "x" ]	# note how many *.c's there were
then
	NDOTC=1
else
	NDOTC=
fi
if [ "$CONLY" ]		# run lint1 on *.c's only producing *.ln's
then
	for i in $FILES
	do
		case $i in
		*.c)	T=`basename $i .c`.ln
			if [ "$NDOTC" ]
			then
				echo $i:
			fi
			($CC_CMD $CCF $i | $LDIR/lint1 $LINTF -H$HOUT $i >$T) 2>&1
			$LDIR/lint2 $LINT2F -H$HOUT
			rm -f $HOUT;;
		esac
	done
else			# send all *.c's through lint1 run all through lint2
	rm -f $TOUT $HOUT
	for i in $FILES
	do
		case $i in
		*.ln)	cat <$i >>$TOUT;;
		*.c)	if [ "$NDOTC" ]
			then
				echo $i:
			fi
			($CC_CMD $CCF $i|$LDIR/lint1 $LINTF -H$HOUT $i >>$TOUT)2>&1;;
		esac
	done
	if [ "$LLIB" ]
	then
		cp $TOUT $LLIB
	fi
	if [ "$DEFL" ]
	then
		if [ "$NAMESPACE" -a "$DEFL"="$LLDIR/llib-lc.ln" ]
		then
			if [ -f /usr/include/nfs/nfs.h ]
			then
				CCF="$CCF -DNFS"
			fi
			if [ -f /usr/include/netdb.h ]
			then
				CCF="$CCF -D_LANLINK"
			fi
			if [ -f /usr/include/netinet/in.h ]
			then
				CCF="$CCF -D_LANBLD"
			fi
			$CC_CMD $CCF $LLDIR/llib-lc | $LDIR/lint1 $LINTF -H/dev/null >>$TOUT
		else
			cat <$DEFL >>$TOUT
		fi
	fi
	if [ -s "$TOUT" ]
	then
		if [ -s "$HOUT" ]
		then
			$LDIR/lint2 $LINT2F -T$TOUT -H$HOUT $LINTF
		else
			$LDIR/lint2 $LINT2F -T$TOUT $LINTF
		fi
	fi
fi
rm -f $TOUT $HOUT
