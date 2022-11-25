
# @(#) $Revision: 70.1 $      
trap "rm -f $$sym?ef; exit" 0 1 2 13 15

STDIN="FALSE"

case $# in
0) 	STDIN="TRUE";;	
1)	case $1 in
	*.o)	set $1 $1
	esac
esac

name1=""
name2=""

if [ "$STDIN" = "TRUE" ]
then
        read name1
        read name2
        if [ "$name2" = "" ]
        then
                case "$name1" in
                *.o) list="$name1 $name1";;
                esac
                nm -g $list
        else
                list="$name1 $name2"
                nm -g $list
                while read name1
                do
                        echo "$name1:"
                        nm -g "$name1"
                done
        fi
else
        nm -g $*
fi |

sed '
	/^$/d
	/:$/{
		/\.o:/!d
		s/://
		h
		s/.*/& &/
		p
		d
	}
	/[TD] /{
		s/.* //
		G
		s/\n/ /
		w '$$symdef'
		d
	}
	s/.* //
	G
	s/\n/ /
	w '$$symref'
	d
'
sort -o $$symdef $$symdef
sort -o $$symref $$symref
join $$symref $$symdef | sed 's/[^ ]* *//'
