#! /bin/sh
#ifdef DOMAIN
# @(#) 10.4 LANGUAGE TOOLS Internal $Revision: 70.15 $
#else
# @(#) B.08.30 LANGUAGE TOOLS Internal $Revision: 70.15 $
#endif
#
# apex shell script  
#
if [ -z "$TMPDIR" ]
then
	TMPDIR=/usr/tmp
fi
APEX_TOP=/usr/apex
TOUT=$TMPDIR/tlint.$$		# combined input for second pass
HOUT=$TMPDIR/hlint.$$		# header messages file
HDCK1=$TMPDIR/hdck1.$$		# header file checker .i file
HDCK2=$TMPDIR/hdck2.$$		# stderr output from cpp
#ifdef OSF
LDIR=/usr/apex/lib		# where first & second pass are
LLDIR=/usr/apex/lib		# where lint libraries are found
ADIR=/usr/apex/lib		# apex-specific files
#else
LDIR=$APEX_TOP/lib		# where first & second pass are
LLDIR=$APEX_TOP/lib		# where lint libraries are found
ADIR=$APEX_TOP/lib		# apex-specific files
#endif
PATH=/bin:/usr/bin
CCF="-E -C -D__lint -D__LINT__ -D__APEX__"	# options for the cc command
# options for the lint passes
#ifdef OSF
LINTF="-P -O/usr/apex/lib/origins -S/usr/apex/lib/targets"
#else
LINTF="-P -O$APEX_TOP/lib/origins -S$APEX_TOP/lib/targets"	
#endif
LINT1F=				# options for lint1
LINT2F=				# options for lint2
# options for lintfor1
#ifdef OSF
FLINTF="-P -p/usr/apex/lib/origins -T/usr/apex/lib/targets"
#else
FLINTF="-P -p$APEX_TOP/lib/origins -T$APEX_TOP/lib/targets"
#endif
FILES=				# the *.c and *.ln files in order
NDOTC=				# how many *.c were there
NDOTF=				# how many *.f were there
DEFL=$LLDIR/llib-lc.ln		# the default library to use
SUPLIB=				# supplementary lib (aegis + bsd or sysv)
LLIB=				# lint library file to create
CONLY=				# set for ``compile only''
pre=				# these three variables used for
post=				# handling options with arguments
optarg=				# list variable to add argument to
MODE=				# ANSI vs compatibility
CC=cc
TARGET=
ORIGIN=
DETAIL=
PASOPT=
#ifdef DOMAIN
DOMAIN=TRUE
#else
DOMAIN=
#endif
#ifdef OSF
HDOPT="-O$APEX_TOP/usr/lib/apex/origins -S$APEX_TOP/usr/lib/apex/targets"
HEAD_LIST=$APEX_TOP/headers/db_files
#else
HDOPT="-O$APEX_TOP/lib/origins -S$APEX_TOP/lib/targets"
HEAD_LIST=$APEX_TOP/headers/db_files
#endif

#
trap "rm -f $TOUT $HOUT $HDCK1 $HDCK2; exit 2" 1 2 3 15
#
# First, run through all of the arguments, building lists
#
if [ $# -eq 0 ]
then
	cat $ADIR/usage
	exit 0
fi
for OPT in "$@"
do
	if [ "$optarg" ]
	then
	    case "$optarg" in
		CCF)	CCF="$CCF $pre$OPT"
			if [ "$pre" = "-D" ]
			then
			    PASOPT="$PASOPT -config $OPT"
			fi
			if [ "$pre" = "-I" ]
			then
			    PASOPT="$PASOPT -idir $OPT"
			fi
			;;
		TARGET) LINT2F="$LINT2F -t$OPT"
			TARGET="-t$OPT";;
		ORIGIN) LINT2F="$LINT2F -o$OPT"
			ORIGIN="-o$OPT"
			case "$OPT" in
			    DOMAIN) DOMAIN=TRUE;;
			    Domain) DOMAIN=TRUE;;
			    domain) DOMAIN=TRUE;;
			    *);;
			esac;;
		DETAIL) case "$OPT" in
			    min)	LINT2F="$LINT2F -d1"
					DETAIL="-d1";;
			    max)	LINT2F="$LINT2F -d255"
					DETAIL="-d255";;
			    *)		LINT2F="$LINT2F -d$OPT"
					DETAIL="-d$OPT";;
			esac;;
		SHOW)	case "$OPT" in
			    targets)	LINT2F="$LINT2F -t?";;
			    origins)	LINT2F="$LINT2f -o?";;
			esac;;
		LLIB) LLIB=$OPT;;
	    esac

	    pre=
	    post=
	    optarg=
	    continue
	fi
	case "$OPT" in
	*.c)	FILES="$FILES $OPT"	NDOTC="x$NDOTC";;
	*.f)	FILES="$FILES $OPT"	NDOTF="x$NDOTF";;
	*.ftn)	FILES="$FILES $OPT"	NDOTF="x$NDOTF";;
	*.pas)	FILES="$FILES $OPT";;
	*.ln)	FILES="$FILES $OPT";;
	+*)	;;
	-target) optarg=TARGET
		continue;;
	-origin) optarg=ORIGIN
		continue;;
	-detail) optarg=DETAIL
		continue;;
	-help)	cat $ADIR/usage
		exit 0;;
	-show)	optarg=SHOW
		continue;;
	-cond)	PASOPT="$PASOPT $OPT"
		continue;;
	-*)	OPT=`echo $OPT | sed s/-//`
		while [ "$OPT" ]
		do
			O=`echo $OPT | sed 's/\\(.\\).*/\\1/'`
			OPT=`echo $OPT | sed s/.//`
			case $O in
			p)	LINTF="$LINTF -p"
#ifndef DOMAIN
				CCF="$CCF -Wp,-T"
#endif
				SUPLIB=
				DEFL=$LLDIR/llib-port.ln;;
			n)	SUPLIB=
				DEFL=;;
			c)	CONLY=1;;
			[abhsuvxy]) LINTF="$LINTF -$O";;
			w)	LINTF="$LINTF -$O$OPT"
				HDOPT="$HDOPT -$O$OPT"
				break;;
			g)	CCF="$CCF -$O";;
			O)	FLINTF="$FLINTF -$O"
				CCF="$CCF -$O";;
			Y)	LINTF="$LINTF -n";;
			Z)	LINT2F="$LINT2F -$O$OPT"
				break;;
			N)	LINTF="$LINTF -$O$OPT"
				break;;
			A)	if [ "$O$OPT" = "Aa" ]
				then
					LINTF="$LINTF -A"
					MODE=ANSI
#ifdef DOMAIN
					CCF="$CCF -A ansi"
#else
					CCF="$CCF -$O$OPT"
#endif
				elif [ "$O$OPT" = "Ac" ]
				then
#ifdef DOMAIN
					CCF="$CCF -A nansi"
#else
					CCF="$CCF -$O$OPT"
#endif
				fi
				break;;
			[WIDU]) if [ "$OPT" ]
				then
					CCF="$CCF -$O$OPT"
					if [ "$O" = "D" ]
					then
					    PASOPT="$PASOPT -config $OPT"
					fi
					if [ "$O" = "I" ]
					then
					    PASOPT="$PASOPT -idir $OPT"
					fi
				else
					optarg=CCF
					pre=-$O
				fi
				break;;
			l)	if [ "$OPT" ]
				then
					FILES="$FILES $LLDIR/llib-l$OPT.ln"
				else
					optarg=FILES
					pre=$LLDIR/llib-l
					post=.ln
				fi
				break;;
			o)	if [ "$OPT" ]
				then
					LLIB="$OPT"
				else
					LLIB=
					optarg=LLIB
					pre=
					post=
				fi
				CONLY=1
				break;;
			*)	echo "apex: bad option ignored: $O";;
			esac
		done;;
	*)	echo "apex: file with unknown suffix ignored: $OPT";;
	esac
done

    if [ "$DOMAIN" -a "$DEFL" ]
	then
	    DEFL="$LLDIR/llib-laegis.ln"
	    if [ \( "$SYSTYPE" = "bsd4.2" \) -o \
		 \( "$SYSTYPE" = "bsd4.3" \) ]
	    then
		SUPLIB="$SUPLIB $LLDIR/llib-lbsd.ln"
	    fi
	    if [ \( "$SYSTYPE" = "sys5" \) -o \
		 \( "$SYSTYPE" = "sys5.3" \) ]
	    then
		SUPLIB="$SUPLIB $LLDIR/llib-lsysv.ln"
	    fi
# add the GPR database library to the list of supplemental libraries
	    if [ -f $LLDIR/llib-lgpr.ln ]
	    then
		SUPLIB="$SUPLIB $LLDIR/llib-lgpr.ln"
	    fi
# add the PHIGS database library to the list of supplemental libraries
	    if [ -f $LLDIR/llib-lphigs.ln ]
	    then
		SUPLIB="$SUPLIB $LLDIR/llib-lphigs.ln"
	    fi

	fi

#
# Second, walk through the FILES list, running all .c's through
# lint1, and just adding all .ln's to the running result
#
if [ "$MODE" != "ANSI" ]  # avoid polluting ANSI namespace
then
	CCF="$CCF -Dlint"
fi
if [ "$NDOTC" != "x" ]	# note how many *.c's there were
then
	NDOTC=1
else
	NDOTC=
fi
if [ "$LLIB" ]
then
	rm -f $LLIB
fi
if [ "$CONLY" ]		# run lint1 on *.c's only producing *.ln's
then
	for i in $FILES
	do
		HEADING_FLAG=""
		case $i in
		*.c)	T=`basename $i .c`.ln
			if [ ! \( -f $i -a -r $i \) ]
			then
			    echo "No source file $i"
			else
			    rm -f $T
			    $CC $CCF $i >$HDCK1 2>$HDCK2
			    $LDIR/missing_h <$HDCK2 | cat $HDCK1 - | $LDIR/hdck $HDOPT $DETAIL $ORIGIN $TARGET -T$T $HEAD_LIST 
			    if [ $? -eq 1 ]
			    then
				HEADING_FLAG="-f"
			    fi
#ifdef DOMAIN
			    $CC $CCF $i >$HDCK1 
#endif
			    (cat $HDCK1 | $LDIR/lint1 $HEADING_FLAG $LINT1F $LINTF -H$HOUT $i >>$T) 2>&1
			    if [ $? -ne 0 ]
			    then
				rm $T
			    fi
			    if [ -s $HOUT ]
			    then
			    $LDIR/lint2 $LINT2F -H$HOUT
			    fi
			    rm -f $HOUT
			fi
			if [ "$LLIB" ]
			then
				cat $T >> $LLIB
				if [ $? -eq 0 ]
				then
				    rm -f $T
				fi
			fi;;

		*.f)	$LDIR/lintfor1 $FLINTF - $i;;
		*.ftn)	$LDIR/lintfor1 $FLINTF - $i;;
		*.pas)	T=`basename $i .pas`.ln
			if [ ! \( -f $LDIR/lintpas1 -a -x $LDIR/lintpas1 \) ]
			then
			    echo "Domain Pascal checking not available"
			else
			    $LDIR/lintpas1 $i $PASOPT >$T
			fi
			if [ "$LLIB" ]
			then
				cat $T >> $LLIB
				if [ $? -eq 0 ]
				then
				    rm -f $T
				fi
			fi;;
		esac
	done
else			# send all *.c's through lint1 run all through lint2
	rm -f $TOUT $HOUT
	for i in $FILES
	do
		HEADING_FLAG=""
		case $i in
		*.ln)	cat <$i >>$TOUT;;
		*.c)	if [ ! \( -f $i -a -r $i \) ]
			then
			    echo "No source file $i"
			else
			    $CC $CCF $i >$HDCK1 2>$HDCK2
			    $LDIR/missing_h <$HDCK2 | cat $HDCK1 - | $LDIR/hdck $HDOPT $DETAIL $ORIGIN $TARGET -T$TOUT $HEAD_LIST 
			    if [ $? -eq 1 ]
			    then
			    HEADING_FLAG="-f"
			    fi
#ifdef DOMAIN
			    $CC $CCF $i >$HDCK1 
#endif
			    (cat $HDCK1|$LDIR/lint1 $HEADING_FLAG $LINT1F $LINTF -H$HOUT $i >>$TOUT)2>&1
			fi;;
		*.f)	$LDIR/lintfor1 $FLINTF - $i
			T=`basename $i .f`.ln
			cat $T >>$TOUT
			rm -f $T;;
		*.ftn)	$LDIR/lintfor1 $FLINTF - $i
			T=`basename $i .ftn`.ln
			cat $T >>$TOUT
			rm -f $T;;
		*.pas)	if [ ! \( -f $LDIR/lintpas1 -a -x $LDIR/lintpas1 \) ]
			then
			    echo "Domain Pascal checking not available"
			else
			    $LDIR/lintpas1 $i $PASOPT >>$TOUT
			fi;;
		esac
	done
	if [ "$DEFL" ]
	then
		cat <$DEFL >>$TOUT
	fi
	if [ "$SUPLIB" ]
	then
	    for f in $SUPLIB
	    do
		cat $f >>$TOUT
	    done
	fi
	if [ -s "$TOUT" ]
	then
		if [ -s "$HOUT" ]
		then
			$LDIR/lint2 -P $LINT2F -T$TOUT -H$HOUT $LINTF
		else
			$LDIR/lint2 -P $LINT2F -T$TOUT $LINTF
		fi
	fi
fi
rm -f $TOUT $HOUT $HDCK1 $HDCK2
