#!/bin/sh
# @(#) $Revision: 70.2 $       
#
# whodo -- combines info from who(1) and ps(1)'
#

PATH=/bin:/usr/bin
export PATH

date; uname
(
    who -s | awk '{
	sub(".*/", "", $2); 
	printf "%s A %s %s %s %s\n", $2, $1, $3, $4, $5;
    }'
    ps -a  | awk '{
	if (NR > 1 && NF >= 4 && $NF != "<defunct>") 
	{
	    sub(".*/", "", $2);
	    printf "%s B %6s %s %s\n", $2, $1, $3, $4;
	}
    }'
) | sort | awk '{
    if ($2 == "A")
	printf "%-8.8s %-8.8s %3.3s %2.2s %s\n", $3, $1, $4, $5, $6;
    else
	printf "         %-8.8s %-6s %-s\n", $3, $4, $5;
}'
