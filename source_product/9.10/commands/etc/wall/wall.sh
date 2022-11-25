
# @(#) $Revision: 37.1 $    
trap "rm -f /tmp/$$; exit 0" 0 1 2 15
a="`whoami`"
a=`expr "$a" : '\([^ ]*\)'`
echo ''Broadcast Message from $a ''>/tmp/$$
cat>>/tmp/$$
who^sed -e 's/^[^ ]* *\([^ ]*\).*/cat \/tmp\/'$$' >\/dev\/\1/' | sh
if [ "$1" != "fast" ]
then
	sleep 30
fi
