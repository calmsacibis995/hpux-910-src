#!/bin/sh
#	@(#)$Revision: 1.3.109.1 $	$Date: 91/11/19 13:53:24 $
#	bfasum.awk	--	determine the percentage hit of data
##
#	input:	filter.awk output containing BFA total/hit data
#	output:	summary percentage hit of all branches
#	args:	$1 -- the name appearing before "BFA total"
##
sed -e 's/\// /g' |
awk 'BEGIN	{ hits = misses = total = 0 }
		{ hits += $3 ; total += $4 }
END		{ percent = (hits*100.0)/total;
		  printf("'$1' BFA total:	%8.2f%%\t\t(%d/%d)\n", percent,hits,total);
		}'
