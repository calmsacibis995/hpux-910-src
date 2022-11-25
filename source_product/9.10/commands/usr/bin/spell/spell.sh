#!/bin/sh
: bourne shell script

# HPUX_ID: @(#) $Revision: 70.1 $       

#	spell program
# B_SPELL flags, D_SPELL dictionary, F_SPELL input files, H_SPELL history, S_SPELL stop, V_SPELL data for -v
# L_SPELL sed script, I_SPELL -i option to deroff
H_SPELL=${H_SPELL-/usr/lib/spell/spellhist}
V_SPELL=/dev/null
F_SPELL=
B_SPELL=
L_SPELL="sed -e \"/^[.'].*[.'][ 	]*nx[ 	]*\/usr\/lib/d\" -e \"/^[.'].*[.'][ 	]*so[ 	]*\/usr\/lib/d\" -e \"/^[.'][ 	]*so[ 	]*\/usr\/lib/d\" -e \"/^[.'][ 	]*nx[ 	]*\/usr\/lib/d\" "
for A in $*
do
	case $A in
	-v)	B_SPELL="$B_SPELL -v"
		if [ -z "$TMPDIR" ]
		then tmp="/tmp"
		else tmp=$TMPDIR
		fi
		trap "/bin/rm -f $tmp/spell.$$; exit" 0 1 2 13 15
		V_SPELL=$tmp/spell.$$
		;;
	-a)	: ;;
	-b) 	D_SPELL=${D_SPELL-/usr/lib/spell/hlistb}
		B_SPELL="$B_SPELL -b" ;;
	-x)	B_SPELL="$B_SPELL -x" ;;
	-l)	L_SPELL="cat" ;;
	+*)	if [ "$FIRSTPLUS" = "+" ]
			then	echo "multiple + options in spell, all but the last are ignored" >&2
		fi;
		FIRSTPLUS="$FIRSTPLUS"+
		if  LOCAL=`expr $A : '+\(.*\)' 2>/dev/null`;
		then if test ! -r $LOCAL;
			then echo "spell cannot read $LOCAL" >&2; EXIT_SPELL=exit;
		     fi
		else echo "spell cannot identify local spell file" >&2; EXIT_SPELL=exit;
		fi ;;
	-i)	I_SPELL="-i" ;;
	-*)	echo "Illegal option: "$A >&2
		echo "Usage: spell [-v] [-b] [-x] [-l] [-i] [+local_file] [files]" >&2
		exit
		;;
	*)	F_SPELL="$F_SPELL $A" ;;
	esac
	done
${EXIT_SPELL-:}
L_SPELL="$L_SPELL $F_SPELL"
eval $L_SPELL |\
 deroff -w $I_SPELL |\
 /bin/sort -u +0f +0  2> /dev/null |\
 /usr/lib/spell/spellprog ${S_SPELL-/usr/lib/spell/hstop} 1 |\
 /usr/lib/spell/spellprog ${D_SPELL-/usr/lib/spell/hlista} $V_SPELL $B_SPELL -l${LOCAL-/dev/null}
case $V_SPELL in
/dev/null)	exit
esac
sed '/^\./d' $V_SPELL | /bin/sort -u +0f +0 2> /dev/null 
