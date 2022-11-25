#! /bin/sh
# @(#) $Revision: 56.6 $
group=
remove_file=
case $# in
	0) file=/tmp/cwall$$		# no params, must read from stdin
	   remove_file=yes
	   ;;
	1) if [ $1 = "-g" ]	# 1 param => file to read
	   then
		echo "Usage: cwall [-g group] [file]" 1>&2
		exit 1
	   else
		file=$1
	   fi
	   ;;
	2) if [ $1 != "-g" ]	# 2 params => -g with no file
	   then
		echo "Usage: cwall [-g group] [file]" 1>&2
		exit 1
	   else
		group=$2
		file=/tmp/cwall$$
		remove_file=yes
	   fi
	   ;;
	3) if [ $1 != "-g" ]	# 3 params => -g with file to read
	   then
		echo "Usage: cwall [-g group] [file]" 1>&2
		exit 1
	   else
		group=$2
		file=$3
	   fi
	   ;;
	*) echo "Usage: cwall [-g group] [file]" 1>&2
	   exit 1
	   ;;
esac

if [ "$remove_file" ]		# read from stdin
then
	cat > $file
fi

for i in `cnodes`
do
	if [ "$group" ]
	then
		remsh $i /etc/wall -g $group <$file &
	else
		remsh $i /etc/wall <$file &
	fi
done

if [ "$remove_file" ]
then
	/bin/rm -f $file
fi
exit 0
