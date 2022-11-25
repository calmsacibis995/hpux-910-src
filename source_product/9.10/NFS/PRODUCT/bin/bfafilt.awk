#!/bin/sh
#	@(#)$Revision: 1.3.109.1 $	$Date: 91/11/19 13:53:04 $
#	filter.awk	--	produce summary file output from BFA input
##
#	input:	data from bfarpt -f
#	output:	summary data of the format:
# module:	hit%		total_branches		hit_branches
##

awk 'BEGIN		{ filename = "" }
/Source file name/	{ filename = $5 }
/Branches/		{ if (filename != "") {
			    print filename ": " $4 " " $3 " " $2 " " $3 "/" $2
			    filename = "UNKNOWN"
			    }
			}
'
