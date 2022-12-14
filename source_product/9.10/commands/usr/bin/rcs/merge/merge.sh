#! /bin/sh
# Three-way file merge. 

#  Create what string for merge shell script.  MUST HAVE SPACE
 # $Revision: 70.3 $

# use: merge [-p] file1 file2 file3 [mark1 mark3]
# Effect: incorporates all changes that lead from file2 to file3 into file1.
# Result goes to std. output if -p is present, into file1 otherwise.
# Overlapping changes are delimited as follows:
# <<<<<<< file1
# lines in file1
# =======
# lines in file3
# >>>>>>> file3
# If mark1 and mark3 are given the delimiting lines <<<<.. and >>>>...
# contain mark1 and mark3 instead of the names of file1 and file3.
# A warning is printed if there are overlaps.

PATH=/bin:/usr/bin:.
DIFF3=/usr/lib/diff3prog
p=0
case $1 in
-p)
        p=$1
	shift;;
esac


if test $# -ge 3
then
        if test -f $1 -a -f $2 -a -f $3
        then
                trap "rm -f /tmp/d3[abc]$$" 0 1 2 13 15
		# The -M option causes diff to not cut off identical
		# lines at the ends of the files.
                diff -M $1 $3 >/tmp/d3a$$
                diff -M $2 $3 >/tmp/d3b$$
		if [ ! -f /tmp/d3a$$ -o ! -f /tmp/d3b$$ ]
		then
			echo "Failed to get needed diffs" 1>&2
			exit 1
		fi
                $DIFF3 -E /tmp/d3[ab]$$ $1 $2 $3 $4 $5 > /tmp/d3c$$
                r=$?
                if test $r != 0
                then
                        echo Warning: $r overlaps during merge. 1>&2
                fi
		if [ ! -f /tmp/d3c$$ ]
		then
			echo "Failed to merge diffs - lost temp file"
			exit 1
		fi
                if test $p != 0
                then
                        echo '1,$p\nq'  >> /tmp/d3c$$ || exit 1
                else
                        echo 'w\nq' >> /tmp/d3c$$ || exit 1
                fi
		if ed - $1 < /tmp/d3c$$
		then
			:
		else
			echo "Edit failed" 1>&2
			exit 1
		fi
                exit 0
        else
                echo "Cannot open $1, $2, or $3" 1>&2
        fi
fi
echo "usage: merge [-p] file1 file2 file3" 1>&2
exit 1
